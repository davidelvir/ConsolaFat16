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
PartitionTable pt[4];
void findRoot()
{

    fseek(root, 0x1BE, SEEK_SET);               // go to partition table start
    fread(pt, sizeof(PartitionTable), 4, root); // read all four entries

    fseek(root, 512 * pt[0].start_sector, SEEK_SET);
    fread(&bs, sizeof(Fat16BootSector), 1, root);

    fseek(root, (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);

    printf("INicial root %d\n", (int)ftell(root));
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

        printf("Read pointer at 0x%X\n", (unsigned int)ftell(in));
        //findRoot();
        printf("Root pointer at 0x%X\n", (unsigned int)ftell(root));
    }
}
void ls_l()
{
    int i;
    in = root;
    printf("Read pointer00 at 0x%X\n", (unsigned int)ftell(in));
    for (i = 0; i < bs.root_dir_entries; i++)
    {

        fread(&entry, sizeof(entry), 1, in);
        /*if (i == 1)
        {
            testRead = entry;
        }*/
        print_file_info(&entry);
    }

    printf("Read pointer at 0x%X\n", (unsigned int)ftell(in));
    //findRoot();
    printf("Root pointer at 0x%X\n", (unsigned int)ftell(root));
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
            print_file_info2(&testRead);
        }
    }
    //findRoot();
    return ret;
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
    printf("Read root2 at 0x%X\n", (unsigned int)ftell(root));
    printf("Read pointer2 at 0x%X\n", (unsigned int)ftell(read));

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
    printf("ROOT after cat %d\n",(int)ftell(root));
}
void fat_read_dir(FILE *readD, unsigned long data_start, unsigned long cluster_size, unsigned short cluster)
{
    Fat16Entry dir;
    Fat16Entry dir2;
    Fat16Entry dir3;
    printf("Cluster %d\n", cluster);
    fseek(readD, data_start + cluster_size * (cluster - 2), SEEK_SET);

    //int tem = 0;
    //printf("break 1\n");
    fread(&dir, sizeof(Fat16Entry), 1, readD);
    fread(&dir2, sizeof(Fat16Entry), 1, readD);
    fread(&dir3, sizeof(Fat16Entry), 1, readD);

    printf("Dir 1 name: %.8s\n", dir.filename);
    printf("Dir 2 name: %.8s\n", dir2.filename);
    printf("Dir 3 name: %.8s\n", dir3.filename);
    /*printf("break 2\n");
    while (dir[tem].attributes != 0x00)
    {
        tem++;
        printf("break 1\n");
        fread(&dir[tem], sizeof(Fat16Entry), 1, readD);
        
    }
    for (int i = 0; i < tem; i++)
    {
        printf("Dir %d name: %.8s\n", i, dir[tem].filename);
    }*/
    printf("This is a dir!\n");
    printf("Read pointer3 at 0x%X\n", (unsigned int)ftell(readD));

    //rewind(fatStart);
}
int main()
{

    fatStart = fopen("test.img", "rb");
    root = fopen("test.img", "rb");
    FILE *read = fopen("test.img", "rb");
    findRoot();
    rootdir = ftell(root);
    int vivo = 1;
    char word[256] = {};
    char cat[8] = {};
    char cd[8] = {};
    for (int i = 0; i < 8; i++)
    {
        cat[i] = ' ';
        cd[i] = ' ';
    }
    while (vivo == 1)
    {
        printf(":>");
        fgets(word, sizeof(word), stdin);
        if (word[0] == 'l' && word[1] == 's' && word[2] == ' ' && word[3] == '-' && word[4] == 'l')
        {
            if (ftell(root) != rootdir)
            {
                printf("NO ESTAS EN ROOT!\n");
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
                printf("NO ESTAS EN ROOT!\n");
            }
            else
            {
                findRoot();
                ls();
            }
        }
        else if (word[0] == 'c' && word[1] == 'a' && word[2] == 't')
        {

            findRoot();
            int tem = 4;
            int tem2 = 0;
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

            if (find_file(cat) == 1)
            {
                printf("Entrada cat\n");
                FILE *a = fopen("a.txt", "wb");
                printf("break 1\n");
                fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
                //printf("pseudo break 2\n");
                printf("Fat start at %ld\n", ftell(fatStart));

                //printf("Fat start 2 %d\n",(int)ftell(fatStart));
                fat_read_file(read, a, ftell(fatStart), ftell(in),
                              16384,
                              testRead.starting_cluster,
                              testRead.file_size);

                fclose(a);
            }
        }
        else if (word[0] == 'c' && word[1] == 'd')
        {
            findRoot();
            int tem = 3;
            int tem2 = 0;
            //vivo = 2;
            printf("cd root %d\n", (int)ftell(root));
            while (tem < 11 && word[tem] != '.')
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
            for (int i = 0; i < 8; i++)
            {
                printf("%c, %d\n", cd[i], i);
            }
            if (find_file(cd) == 1)
            {
                //printf("hola\n");
                fseek(fatStart, 0x1BE + (512 * pt[0].start_sector) + sizeof(Fat16BootSector) + 66, SEEK_SET);
                printf("Fat start at %ld\n", ftell(fatStart));
                fat_read_dir(read, ftell(in), 16384, testRead.starting_cluster);
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