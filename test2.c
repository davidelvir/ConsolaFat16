#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct
{
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    unsigned int start_sector;
    unsigned int length_sectors;
} __attribute((packed)) PartitionTable;

typedef struct
{
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

typedef struct
{
    unsigned char filename[8];
    unsigned char ext[3];
    unsigned char attributes;
    unsigned char reserved[10];
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned short starting_cluster;
    unsigned int file_size;
} __attribute((packed)) Fat16Entry;
void print_file_info2(Fat16Entry *entry)
{
    switch (entry->attributes)
    {
    case 0x00:
        return; // unused entry
    case 0xE5:
        printf("Deleted file: [?%.7s.%.3s]\n", entry->filename + 1, entry->ext);
        return;
    case 0x05:
        printf("File starting with 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename + 1, entry->ext);
        break;
    case 0x10:
        printf("%.8s.%.3s\n", entry->filename, entry->ext);
        break;
    case 0x20:
        printf("%.8s.%.3s\n", entry->filename, entry->ext);
        break;
    default:
        printf("File attribute not found\n");
    }
}
void print_file_info(Fat16Entry *entry)
{
    switch (entry->attributes)
    {
    case 0x00:
        return; // unused entry
    case 0xE5:
        printf("Deleted file: [?%.7s.%.3s]\n", entry->filename + 1, entry->ext);
        return;
    case 0x05:
        printf("File starting with 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename + 1, entry->ext);
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
}

FILE *root;
FILE *fatStart;
FILE *in;
int rootdir;
Fat16BootSector bs;
Fat16Entry entry;
Fat16Entry testRead;
Fat16Entry read2;
Fat16Entry w;
PartitionTable pt[4];
FILE *write;
char ibuffer[1048576];
void findRoot()
{

    fseek(root, 0x1BE, SEEK_SET);               // go to partition table start
    fread(pt, sizeof(PartitionTable), 4, root); // read all four entries

    fseek(root, 512 * pt[0].start_sector, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, root);

    fseek(root, (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);

    //printf("INicial root %d\n", (int)ftell(root));
}
void Fat_write(){
    //write = fopen("test2.img","wb");
    
    if (rootdir != ftell(root))
    {
        printf("NOT IN ROOT!!\n");
    }
    else
    {
        int i;
        in = root;
        fread(&entry, sizeof(entry), 1, in);
        while(entry.attributes != 0){
            fread(&entry, sizeof(entry), 1, in);
        }
        int new_entry = (int)ftell(in);
        memcpy(&(ibuffer[new_entry]), &w,sizeof(w));

        write = fopen("test2.img","wb");
        fwrite(ibuffer,1,sizeof(ibuffer),write);
        fclose(write);

        /*for (i = 0; i < bs.root_dir_entries; i++)
        {

            fread(&entry, sizeof(entry), 1, in);
            
            //print_file_info2(&entry);
        }
        fseek(write,ftell(in),SEEK_SET);
        printf("Write pointer at 0x%X\n", (unsigned int)ftell(write));
        //printf("Read pointer at 0x%X\n", (unsigned int)ftell(in));
        
        findRoot();*/
        //printf("Write pointer at 0x%X\n", (unsigned int)ftell(write));
        printf("Root pointer at 0x%X\n", (unsigned int)ftell(root));
    }
}
void ls()
{
    if (rootdir != ftell(root))
    {
        printf("NOT IN ROOT!!\n");
    }
    else
    {
        int i;
        in = root;
        for (i = 0; i < bs.root_dir_entries; i++)
        {

            fread(&entry, sizeof(entry), 1, in);
            if (i == 1)
            {
                testRead = entry;
            }
            print_file_info2(&entry);
        }

        //printf("Read pointer at 0x%X\n", (unsigned int)ftell(in));
        findRoot();
        //printf("Root pointer at 0x%X\n", (unsigned int)ftell(root));
    }
}
void ls_l()
{
    int i;
    in = root;
    //printf("Read pointer00 at 0x%X\n", (unsigned int)ftell(in));
    for (i = 0; i < bs.root_dir_entries; i++)
    {

        fread(&entry, sizeof(entry), 1, in);
        /*if (i == 1)
        {
            testRead = entry;
        }*/
        print_file_info(&entry);
    }

    printf("Root pointer  2 at 0x%X\n", (unsigned int)ftell(root));
    findRoot();
    printf("Root pointer  3 at 0x%X\n", (unsigned int)ftell(root));
    //printf("Root pointer at 0x%X\n", (unsigned int)ftell(root));
}
int find_file(char *find)
{
    int i;
    int ret = 0;
    in = root;
    for (i = 0; i < bs.root_dir_entries; i++)
    {

        fread(&entry, sizeof(entry), 1, in);
        if (find[0] == entry.filename[0] && find[1] == entry.filename[1] && find[2] == entry.filename[2] && find[3] == entry.filename[3] && find[4] == entry.filename[4] && find[5] == entry.filename[5] && find[6] == entry.filename[6] && find[7] == entry.filename[7])
        {

            ret = 1;
            testRead = entry;
            //print_file_info2(&testRead);
        }
    }
    /*if(testRead.attributes == 0x10){
        findRoot();
    }*/
    //findRoot();
    return ret;
}
int find_file2(char *find, FILE *readD, unsigned long data_start, unsigned long cluster_size, unsigned short cluster)
{
    int ret = 0;
    Fat16Entry dir;
    //printf("Cluster %d\n", cluster);
    fseek(readD, data_start + cluster_size * (cluster - 2), SEEK_SET);
    fread(&dir, sizeof(Fat16Entry), 1, readD);

    while (dir.attributes != 0x00)
    {

        //printf("file name %.8s\n", dir.filename);
        if (find[0] == dir.filename[0] )
        {
            ret = 1;
            read2 = dir;
            break;
        }
        fread(&dir, sizeof(Fat16Entry), 1, readD);
    }
    //printf("%d\n", ret);
    return ret;
}
void fat_read_file2(FILE *read, FILE *out,
                   unsigned long fat_start,
                   unsigned long data_start,
                   unsigned long cluster_size,
                   unsigned short cluster,
                   unsigned long file_size)
{
    int ret = ftell(root);
    unsigned char buffer[4096] = {};
    size_t bytes_read, bytes_to_read,
        file_left = file_size, cluster_left = cluster_size;

    // Go to first data cluster

    //printf("Read pointer2 at 0x%X\n", (unsigned int)ftell(read));
    fseek(read, data_start + cluster_size * (cluster - 2), SEEK_SET);
    //printf("Read root2 at 0x%X\n", (unsigned int)ftell(root));
    /*printf("Read pointer2 at 0x%X\n", (unsigned int)ftell(read));*/

    while (file_left > 0 && cluster != 0xFFFF)
    {

        bytes_to_read = sizeof(buffer);
        if (bytes_to_read > file_left)
            bytes_to_read = file_left;
        if (bytes_to_read > cluster_left)
            bytes_to_read = cluster_left;
        bytes_read = fread(buffer, 1, bytes_to_read, read);
        fwrite(buffer, 1, bytes_read, out);
        //printf("Copied %d bytes\n", (int)bytes_read);

        printf("%.4096s", buffer);
        cluster_left -= bytes_to_read;
        file_left -= bytes_read;
        //printf("Bytes left to read %d\n", (int)cluster_left);
        if (cluster_left == 0)
        {
            fseek(read, fat_start + cluster * 2, SEEK_SET);
            printf("cluster seek %d\n", (int)ftell(read));
            fread(&cluster, 2, 1, read);

            printf("End of cluster reached, next cluster %d\n", cluster);
            fseek(read, data_start + cluster_size * (cluster - 2), SEEK_SET);
            cluster_left = cluster_size; // reset cluster byte counter
        }
    }
    //printf("Read root 3 at 0x%X\n", (unsigned int)ftell(root));
    //findRoot();
    //printf("ROOT after cat %d\n",(int)ftell(root));
}
void fat_read_file(FILE *read, FILE *out,
                   unsigned long fat_start,
                   unsigned long data_start,
                   unsigned long cluster_size,
                   unsigned short cluster,
                   unsigned long file_size)
{
    unsigned char buffer[4096] = {};
    size_t bytes_read, bytes_to_read,
        file_left = file_size, cluster_left = cluster_size;

    // Go to first data cluster

    //printf("Read pointer2 at 0x%X\n", (unsigned int)ftell(read));
    fseek(read, data_start + cluster_size * (cluster - 2), SEEK_SET);
    //printf("Read root2 at 0x%X\n", (unsigned int)ftell(root));
    //printf("Read pointer2 at 0x%X\n", (unsigned int)ftell(read));

    while (file_left > 0 && cluster != 0xFFFF)
    {

        bytes_to_read = sizeof(buffer);
        if (bytes_to_read > file_left)
            bytes_to_read = file_left;
        if (bytes_to_read > cluster_left)
            bytes_to_read = cluster_left;
        bytes_read = fread(buffer, 1, bytes_to_read, read);
        fwrite(buffer, 1, bytes_read, out);
        printf("Copied %d bytes\n", (int)bytes_read);

        printf("%.4096s", buffer);
        cluster_left -= bytes_to_read;
        file_left -= bytes_read;
        printf("Bytes left to read %d\n", (int)cluster_left);
        if (cluster_left == 0)
        {
            fseek(read, fat_start + cluster * 2, SEEK_SET);
            printf("cluster seek %d\n", (int)ftell(read));
            fread(&cluster, 2, 1, read);

            printf("End of cluster reached, next cluster %d\n", cluster);
            fseek(read, data_start + cluster_size * (cluster - 2), SEEK_SET);
            cluster_left = cluster_size; // reset cluster byte counter
        }
    }
    findRoot();
    //printf("ROOT after cat %d\n",(int)ftell(root));
}
void fat_read_dir(FILE *readD, unsigned long data_start, unsigned long cluster_size, unsigned short cluster)
{
    Fat16Entry dir;
    //printf("Cluster %d\n", cluster);
    fseek(readD, data_start + cluster_size * (cluster - 2), SEEK_SET);
    fread(&dir, sizeof(Fat16Entry), 1, readD);
    while (dir.attributes != 0x00)
    {
        print_file_info(&dir);
        fread(&dir, sizeof(Fat16Entry), 1, readD);
    }
    /*printf("This is a dir!\n");
    printf("Read pointer3 at 0x%X\n", (unsigned int)ftell(readD));*/
}
void move_to_dir(FILE *readD, unsigned long data_start, unsigned long cluster_size, unsigned short cluster)
{
    Fat16Entry dir;
    //printf("Cluster %d\n", cluster);
    fseek(readD, data_start + cluster_size * (cluster - 2), SEEK_SET);
    fread(&dir, sizeof(Fat16Entry), 1, readD);
    while (dir.attributes != 0x00)
    {
        //print_file_info(&dir);
        fread(&dir, sizeof(Fat16Entry), 1, readD);
    }
    //printf("This is a dir!\n");
    //printf("Read pointer3 at 0x%X\n", (unsigned int)ftell(readD));
}
void return_to_dir(FILE *readD, unsigned long data_start, unsigned long cluster_size, unsigned short cluster)
{
    Fat16Entry dir;
    Fat16Entry dir2;
    //printf("%.8s \n", (char *)testRead.filename);
    //printf("Cluster %d\n", cluster);
    fseek(readD, data_start + cluster_size * (cluster - 2), SEEK_SET);
    fread(&dir, sizeof(Fat16Entry), 1, readD);
    fread(&dir2, sizeof(Fat16Entry), 1, readD);
    //printf("Return cluster: %d\n", dir2.starting_cluster);
    fseek(root, data_start + cluster_size * (dir2.starting_cluster - 2), SEEK_SET);
    //printf("Root after return at 0x%X\n", (unsigned int)ftell(root));
    /*while (dir.attributes != 0x00)
    {
        //print_file_info(&dir);
        fread(&dir, sizeof(Fat16Entry), 1, readD);
    }*/
}
int main()
{

    unsigned char filename[8] = {'P','R','U','E','B','A',' ',' '};
    unsigned char ext2[3] = {'T','X','T'};
    unsigned char attributes = 0x20;
    unsigned char reserved[10] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
    unsigned short modify_time = 2;
    unsigned short modify_date = 2;
    unsigned short starting_cluster = 30;
    unsigned int file_size = 30;

    fatStart = fopen("test2.img", "rb");
    root = fopen("test2.img", "rb");
    FILE *read = fopen("test2.img", "rb");
    findRoot();
    rootdir = ftell(root);

    FILE *copy = fopen("test2.img","rb");
    
    

    fread(ibuffer,sizeof(ibuffer),1,copy);
    fclose(copy);
    memcpy(w.filename,filename,sizeof(filename));
    memcpy(w.ext,ext2,sizeof(ext2));
    memcpy(&w.attributes,&attributes,sizeof(attributes));
    memcpy(w.reserved,reserved,sizeof(reserved));
    memcpy(&w.modify_time,&modify_time,sizeof(modify_time));
    memcpy(&w.modify_date,&modify_date,sizeof(modify_date));
    memcpy(&w.starting_cluster,&starting_cluster,sizeof(starting_cluster));
    memcpy(&w.file_size,&file_size,sizeof(file_size));

    //Fat_write();
    //fwrite(&w,sizeof(Fat16Entry),1,write);
    
    //fclose(write);
    findRoot();
    //printf("Attribute %u\n",w.attributes);
    int vivo = 1;
    char word[256] = {};
    char cat[8] = {};
    char cat2[8] = {};
    char ext[3] = {};
    char cd[8] = {};
    for (int i = 0; i < 8; i++)
    {
        cat[i] = ' ';
        cat2[i] = ' ';
        cd[i] = ' ';
        ext[i] = ' ';
    }
    while (vivo == 1)
    {
        for (int i = 0; i < 8; i++)
        {
            cat[i] = ' ';
            cd[i] = ' ';
        }
        printf("%.8s :>", ftell(root) == rootdir ? "Root" : (char *)testRead.filename);

        fgets(word, sizeof(word), stdin);
        if (word[0] == 'l' && word[1] == 's' && word[2] == ' ' && word[3] == '-' && word[4] == 'l')
        {
            if (ftell(root) != rootdir)
            {
                /*fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
                printf("Fat start at %ld\n", ftell(fatStart));*/
                fat_read_dir(read, ftell(in), 16384, testRead.starting_cluster);
                //printf("NO ESTAS EN ROOT!\n");
            }
            else
            {
                findRoot();
                ls_l();
            }
        }
        else if (word[0] == 'l' && word[1] == 's')
        {
            if (ftell(root) != rootdir)
            {
                /*fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
                printf("Fat start at %ld\n", ftell(fatStart));*/
                fat_read_dir(read, ftell(in), 16384, testRead.starting_cluster);
            }
            else
            {
                findRoot();
                ls();
            }
        }
        else if (word[0] == 'c' && word[1] == 'a' && word[2] == 't')
        {
            for (int i = 0; i < 8; i++)
            {
                cat[i] = ' ';
            }

            //findRoot();
            int tem = 4;
            int tem2 = 0;
            int catT = 0;
            //vivo = 2;
            while (tem < 11 && word[tem] != '.')
            {

                if (word[tem] == '\n')
                {
                    cat[tem2] = ' ';
                }
                else
                {
                    cat[tem2] = word[tem];
                }

                tem2++;
                tem++;
            }
            //printf("Char %c\n",word[10]);
            if (word[10] == '.')
            {
                //printf("HOLA\n");
                catT = 1;
            }
            if (ftell(root) == rootdir)
            {
                if (find_file(cat) == 1 && catT == 0)
                {
                    //printf("Entrada cat\n");
                    FILE *a = fopen("a.txt", "wb");
                    //printf("break 1\n");
                    fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
                    
                    fat_read_file(read, a, ftell(fatStart), ftell(in),
                                  16384,
                                  testRead.starting_cluster,
                                  testRead.file_size);

                    fclose(a);
                }
                else
                {
                    printf("Aqui va el otro cat\n");
                    findRoot();
                }
            }
            else
            {
                
                printf("cat subdir\n");
                if (find_file2(cat, read, ftell(in), 16384, testRead.starting_cluster) == 1)
                {
                    printf("encontrado!\n");
                    //printf("Entrada cat\n");
                    FILE *a = fopen("a.txt", "wb");
                    //printf("break 1\n");
                    fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
                    //printf("pseudo break 2\n");
                    //printf("Fat start at %ld\n", ftell(fatStart));

                    //printf("Fat start 2 %d\n",(int)ftell(fatStart));
                    fat_read_file2(read, a, ftell(fatStart), ftell(in),
                                  16384,
                                  read2.starting_cluster,
                                  read2.file_size);

                    fclose(a);
                }
            }
        }
        else if (word[0] == 'c' && word[1] == 'd')
        {
            //findRoot();
            int tem = 3;
            int tem2 = 0;
            //vivo = 2;
            printf("cd root %d\n", (int)ftell(root));
            while (tem < 11)
            {

                if (word[tem] == '\n')
                {
                    cd[tem2] = ' ';
                }
                else
                {
                    cd[tem2] = word[tem];
                }

                tem2++;
                tem++;
            }
            cd[7] = ' ';
            /*for (int i = 0; i < 8; i++)
            {
                printf("%c, %d\n", cd[i], i);
            }*/
            if ((cd[0] == '.' && cd[1] == '.' && cd[2] == '/' && ftell(root) != rootdir) || (cd[0] == '.' && cd[1] == '.' && ftell(root) != rootdir))
            {
                //printf("break 1\n");
                return_to_dir(read, ftell(in), 16384, testRead.starting_cluster);
                findRoot();
            }
            else if ((cd[0] == '.' && cd[1] == '.' && cd[2] == '/' && ftell(root) == rootdir) || (cd[0] == '.' && cd[1] == '.' && ftell(root) == rootdir))
            {
                //printf("break 2\n");
                findRoot();
            }
            else
            {
                if (find_file(cd) == 1)
                {
                    //printf("hola\n");
                    /*fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
                    printf("Fat start at %ld\n", ftell(fatStart));*/
                    move_to_dir(read, ftell(in), 16384, testRead.starting_cluster);
                }
            }
        }
        else if (word[0] == 'e' && word[1] == 'x' && word[2] == 'i' && word[3] == 't')
        {
            printf("ADIOS!\n");
            vivo = 0;
        }
    }
    /*ls_l();
    //findRoot();
    //comienzo del fat

    fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
    printf("Fat start at %ld\n", ftell(fatStart));
    print_file_info(&testRead);
    fat_read_file(read, a, ftell(fatStart), ftell(in),
                  16384,
                  testRead.starting_cluster,
                  testRead.file_size);*/

    fclose(fatStart);
    fclose(root);
    fclose(in);
    fclose(read);
    return 0;
}