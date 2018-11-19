#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct {
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    unsigned int start_sector;
    unsigned int length_sectors;
} __attribute((packed)) PartitionTable;

typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_dir_entries;
    unsigned short total_sectors_short; // if zero, later field is used
    unsigned char media_descriptor;
    unsigned short fat_size_sectors;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned int hidden_sectors;
    unsigned int total_sectors_long;
    
    unsigned char drive_number;
    unsigned char current_head;
    unsigned char boot_signature;
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat16BootSector;

typedef struct {
    unsigned char filename[8];
    unsigned char ext[3];
    unsigned char attributes;
    unsigned char reserved[10];
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned short starting_cluster;
    unsigned int file_size;
} __attribute((packed)) Fat16Entry;

void print_everything(Fat16Entry *entry){
    printf("Nombre: %.8s.%.3s\n",entry->filename,entry->ext);
}
void print_file_info(Fat16Entry *entry) {
    switch(entry->attributes) {
    case 0x00:
        return; // unused entry
    case 0xE5:
        printf("Deleted file: [?%.7s.%.3s]\n", entry->filename+1, entry->ext);
        return;
    case 0x05:
        printf("File starting with 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename+1, entry->ext);
        break;
    case 0x10:
        printf("Directory: [%.8s.%.3s]\n", entry->filename, entry->ext);
        break;
    case 0x20:
        printf("File: [%.8s.%.3s]\n", entry->filename, entry->ext);
        break;
    default:
        printf("File attribute not found\n");
    }
    
    printf("  Modified: %04d-%02d-%02d %02d:%02d.%02d    Start: [%04X]    Size: %d\n", 
        1980 + (entry->modify_date >> 9), (entry->modify_date >> 5) & 0xF, entry->modify_date & 0x1F,
        (entry->modify_time >> 11), (entry->modify_time >> 5) & 0x3F, entry->modify_time & 0x1F,
        entry->starting_cluster, entry->file_size);

    //printf("Este es el tipo %u\n",entry->attributes);
    
    //printf("Tamaño de unsigned char %d\n",sizeof(unsigned char));
}

void fat_read_file(FILE * in, FILE * out,
                   unsigned int fat_start, 
                   unsigned int data_start, 
                   unsigned int cluster_size, 
                   unsigned short cluster, 
                   unsigned int file_size) {
    unsigned char buffer[4096];
    size_t bytes_read, bytes_to_read,
           file_left = file_size, cluster_left = cluster_size;

    // Go to first data cluster
    //printf("FIle size %d y cluster size %d\n",file_size,cluster_size);
    printf("Num de clusters %f\n",ceil(((double)file_size/cluster_size)) > 0 ? ceil(((double)file_size/cluster_size)):1);
    fseek(in, data_start + cluster_size * (cluster-2), SEEK_SET);
    int test = 0;
    int tem = cluster;
    // Read until we run out of file or clusters

    while(test < ceil(((double)file_size/cluster_size)) > 0 ? ceil(((double)file_size/cluster_size)):1) {
        bytes_to_read = sizeof(buffer);
        
        // don't read past the file or cluster end
        if(bytes_to_read > file_left)
            bytes_to_read = file_left;
        if(bytes_to_read > cluster_left)
            bytes_to_read = cluster_left;
        
        // read data from cluster, write to file
        bytes_read = fread(buffer, 1, bytes_to_read, in);
        fwrite(buffer, 1, bytes_read, out);
        printf("Copied %d bytes\n", (int)bytes_read);
        
        // decrease byte counters for current cluster and whole file
        cluster_left -= bytes_read;
        file_left -= bytes_read;

        // if we have read the whole cluster, read next cluster # from FAT        
        if(cluster_left == 0) {
            //fseek(in, fat_start + cluster*2, SEEK_SET);
            //fread(&cluster, 2, 1, in);
            cluster ++;
            printf("End of cluster reached, next cluster %d\n", cluster);
            
            //fseek(in, data_start + cluster_size * (cluster-2), SEEK_SET);
            cluster_left = cluster_size; // reset cluster byte counter
        }
        test++;
    }
}
int main() {
    FILE * in = fopen("test2.img", "rb");
    FILE * out = fopen("a.txt","wb");
    int i;
    PartitionTable pt[4];
    Fat16BootSector bs;
    Fat16Entry entry;
    Fat16Entry entry2;
   
    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    printf("Now at 0x%X\n",(unsigned int)ftell(in));
    fread(pt, sizeof(PartitionTable), 4, in); // read all four entries
     
    for(i=0; i<4; i++) {        
        if(pt[i].partition_type == 4 || pt[i].partition_type == 6 ||
           pt[i].partition_type == 14) {
            printf("FAT16 filesystem found from partition %d\n", i);
            break;
        }
    }
    
    if(i == 4) {
        printf("No FAT16 filesystem found, exiting...\n");
        return -1;
    }
    
    fseek(in, 512 * pt[i].start_sector, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, in);
    
    printf("Now at 0x%X, sector size %d, FAT size %d sectors, %d FATs\n\n", 
           (unsigned int)ftell(in), bs.sector_size, bs.fat_size_sectors, bs.number_of_fats);
           
    fseek(in, (bs.reserved_sectors-1 + bs.fat_size_sectors * bs.number_of_fats) *
          bs.sector_size, SEEK_CUR);
         
    unsigned char name [8] = {'R','E','A','D','M','E',' ',' '};
    
         //printf("File: [%.8s]\n", name);
    for(i=0; i<bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, in);
        
         //printf("File: [%.8s]\n", entry.filename);
        if(memcmp(name,entry.filename,8) == 0){
            entry2 = entry;

        }
        if(entry.attributes != 0){
            
        printf("File size %u\n",entry.file_size);
        printf("Starting cluster %u\n",entry.starting_cluster);
        }
        print_file_info(&entry);
    }
    //    fseek(in, 512 * pt[i].start_sector, SEEK_SET);

    //fat_read_file(in, out,ftell(in), ftell(in), 16384,  entry2.starting_cluster, entry2.file_size);
    //printf("Sector size %u and Secs/Cluster %u\n",bs.sector_size,bs.sectors_per_cluster);
    
    printf("\nRoot directory read, now at 0x%X\n", (unsigned int)ftell(in));
    rewind(in);
    
    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    printf("Now at 0x%X\n",(unsigned int)ftell(in));
    /* fseek(in, 512 * pt[i].start_sector, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, in);
    
    printf("Now at 0x%X, sector size %d, FAT size %d sectors, %d FATs\n\n", 
           (unsigned int)ftell(in), bs.sector_size, bs.fat_size_sectors, bs.number_of_fats);
          
    Fat16Entry* write;*/
    
   
    fclose(in);
    return 0;
}