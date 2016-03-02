#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;
#define myfirst_sector_of_cluster(cluster, x) ((((mycluster) - 2) * x->mysectors_per_cluster) + myfirst_data_sector)
#define myis_dir(mydir) (mydir->myattribute & 0x10)
#define myis_hidden(mydir) (mydir->myattribute & 0x02)
#define myfirst_cluster(mydir) (mydir->myhigh_first_cluster << 16 | mydir->mylow_first_cluster)
#define mydeleted_cluster(mycluster) (mycluster >= 0x0FFFFFF8)
#define mybad_cluster(mycluster) (mycluster == 0x0FFFFFF7)
#define myunused_cluster(mycluster) (mycluster == 0)
#define myok_cluster(mycluster) (0x00000002 <= mycluster && mycluster <= 0x0FFFFFEF)
#define myLFN_flag 0x0F
#define mydeleted_entry_flag 0xe5
#define mydot_entry_flag 0x2e
typedef struct myfat_extBS_32{
    unsigned int        mytable_size_32;
    unsigned short      myextended_flags;
    unsigned short      myfat_version;
    unsigned int        myroot_cluster;
    unsigned short      myfat_info;
    unsigned short      mybackup_BS_sector;
    unsigned char       myreserved_0[12];
    unsigned char       mydrive_number;
    unsigned char       myreserved_1;
    unsigned char       myboot_signature;
    unsigned int        myvolume_id;
    unsigned char       myvolume_label[11];
    unsigned char       myfat_type_label[8];
}__attribute__((packed)) myfat_extBS_32_t;
typedef struct myfat_BS{
    unsigned char       mybootjmp[3];
    unsigned char       myoem_name[8];
    unsigned short          mybytes_per_sector;
    unsigned char       mysectors_per_cluster;
    unsigned short      myreserved_sector_count;
    unsigned char       mytable_count;
    unsigned short      myroot_entry_count;
    unsigned short      mytotal_sectors_16;
    unsigned char       mymedia_type;
    unsigned short      mytable_size_16;
    unsigned short      mysectors_per_track;
    unsigned short      myhead_side_count;
    unsigned int        myhidden_sector_count;
    unsigned int        mytotal_sectors_32;
    unsigned char       myext_section[54];
}__attribute__((packed)) myfat_BS_t;
typedef struct mydir_table{
    unsigned char       myfilename[11];
    unsigned char       myattribute;
    unsigned char       myreversed;
    unsigned char       mycreation_second;
    unsigned short      mycreation_time;
    unsigned short      mycreation_date;
    unsigned short      mylast_accessed_date;
    unsigned short      myhigh_first_cluster;
    unsigned short      mylast_modified_time;
    unsigned short      mylast_modified_date;
    unsigned short      mylow_first_cluster;
    unsigned int        mysize_of_file;
}__attribute__((packed)) mydir_table_t;
typedef struct myLFN{
    unsigned char       myseq_number;
    unsigned short      myfilename1[5];
    unsigned char       myattribute;
    unsigned char       mytype;
    unsigned char       mychecksum;
    unsigned short      myfilename2[6];
    unsigned short      myreversed;
    unsigned short      myfilename3[2];
}__attribute__((packed)) myLFN_t;

myfat_BS_t *myfat_BS;
myfat_extBS_32_t *myfat_ext_BS;
unsigned int *myfat_table;
unsigned int myroot_dir_sectors, myfirst_data_sector, myfat_size, myfirst_fat_sector, mydata_sectors, mytotal_clusters, myfirst_root_dir_sector, myroot_cluster_32, myfirst_sector_of_cluster, myentry_per_sector;
int myread_BS(int);
int myread_fat(int);
int myread_sector(int, unsigned char *, unsigned int);
int myread_cluster(int, unsigned int, const string &, string&);
int myread_cluster_chain(int, unsigned int, const string &);

int myread_BS(int myfd){
    myfat_BS = new myfat_BS_t;
    unsigned char mybuf[512];
    if(read(myfd, mybuf, 512) <= 0){
        perror("read BS failed");
        return -1;
    }
    memcpy(myfat_BS, mybuf, sizeof(myfat_BS_t));
    myfat_ext_BS = (myfat_extBS_32_t*)myfat_BS->myext_section;
    myfat_size = (myfat_BS->mytable_size_16 == 0) ? myfat_ext_BS->mytable_size_32 : myfat_BS->mytable_size_16;
    myroot_dir_sectors = ((myfat_BS->myroot_entry_count * 32) + (myfat_BS->mybytes_per_sector - 1)) / myfat_BS->mybytes_per_sector;
    myfirst_data_sector = myfat_BS->myreserved_sector_count + (myfat_BS->mytable_count * myfat_size) + myroot_dir_sectors;
    myfirst_fat_sector = myfat_BS->myreserved_sector_count;
    mydata_sectors = myfat_BS->mytotal_sectors_32 - (myfat_BS->myreserved_sector_count + (myfat_BS->mytable_count * myfat_size) + myroot_dir_sectors);
    mytotal_clusters = mydata_sectors / myfat_BS->mysectors_per_cluster;
    myfirst_root_dir_sector = myfirst_data_sector - myroot_dir_sectors;
    myroot_cluster_32 = myfat_ext_BS->myroot_cluster;
    myentry_per_sector = myfat_BS->mybytes_per_sector / 32;
    return 0;
}

int myread_fat(int myfd){
    myfat_table = new unsigned int [myfat_size * myfat_BS->mybytes_per_sector / 4];
    if(lseek(myfd, myfirst_fat_sector * myfat_BS->mybytes_per_sector, SEEK_SET) < 0){
        perror("lseek failed");
        return -1;
    }
    if(read(myfd, (unsigned char*)myfat_table, myfat_size * myfat_BS->mybytes_per_sector) <= 0){
        perror("read fat failed");
        return -1;
    }
    return 0;
}
int myread_sector(int myfd, unsigned char *mysector, unsigned int mysector_num){
    if(lseek(myfd, mysector_num * myfat_BS->mybytes_per_sector, SEEK_SET) < 0){
        perror("lseek failed");
        return -1;
    }
    if(read(myfd, mysector, myfat_BS->mybytes_per_sector) <= 0){
        perror("read sector failed");
        return -1;
    }
    return 0;
}
int mycnt = 0;
string myfilename8_3(const char *mybuf){
    string myfilename = "";
    string myext = "";
    for(int i=0;i<8;i++){
        if(mybuf[i] != ' ')
            myfilename += mybuf[i];
        else break;
    }
    for(int i=8;i<11;i++){
        if(mybuf[i] != ' ')
            myext += mybuf[i];
        else break;
    }
    return (myext == "") ? myfilename : (myfilename + "." + myext);

}

int myread_cluster(int myfd, unsigned int mycluster, const string &mynow_filename, string &mylfn_filename){
    unsigned int myfirst_sector = myfirst_sector_of_cluster(mycluster, myfat_BS);
    unsigned char *mysector = new unsigned char [myfat_BS->mybytes_per_sector];
    int myend = false;
    for(int i=0;i<myfat_BS->mysectors_per_cluster&&!myend;i++){
        if(myread_sector(myfd, mysector, myfirst_sector + i) < 0)
            return -1;
        for(int j=0;j<(int)myentry_per_sector;j++){
            unsigned char *myentry = (mysector + (32 * j));
            if(myentry[0] == 0){
                myend = true;
                mylfn_filename = "";
                break;
            }
            if(myentry[0] == mydot_entry_flag || myentry[0] == mydeleted_entry_flag)
                continue;
            if(myentry[11] == myLFN_flag){ // LFN entry
                myLFN_t *mylfn = (myLFN_t*)myentry;
                string myfilename = "";
                bool myfilename_end =  false;
                for(int k=0;k<5&&!myfilename_end;k++){
                    if(mylfn->myfilename1[k]) myfilename += (char)mylfn->myfilename1[k];
                    else myfilename_end = true;
                }
                for(int k=0;k<6&&!myfilename_end;k++){
                    if(mylfn->myfilename2[k]) myfilename += (char)mylfn->myfilename2[k];
                    else myfilename_end = true;
                }
                for(int k=0;k<2&&!myfilename_end;k++){
                    if(mylfn->myfilename3[k]) myfilename += (char)mylfn->myfilename3[k];
                    else myfilename_end = true;
                }
                mylfn_filename = myfilename + mylfn_filename;
            }else{ //dir table entry
                mydir_table_t *mydir_table = (mydir_table_t*)myentry;
                if(myis_hidden(mydir_table)){
                    mylfn_filename = "";
                    continue;
                }
                string myfilename = (mylfn_filename == "") ? myfilename8_3((char*)mydir_table->myfilename) : mylfn_filename;
                if(myis_dir(mydir_table)){ // directory
                    printf("%s\n", (mynow_filename + myfilename + "/").c_str());
                    myread_cluster_chain(myfd, myfirst_cluster(mydir_table), (mynow_filename + myfilename + "/"));
                    mylfn_filename = "";
                }else{ // file
                    mycnt ++;
                    printf("%s\n", (mynow_filename + myfilename).c_str());
                    mylfn_filename = "";
                }
            }
        }
    }
    return 0;
}

int myread_cluster_chain(int myfd, unsigned int mycluster, const string &mynow_filename){
    string mylfn_filename = "";
    while(myok_cluster(mycluster)){
        myread_cluster(myfd, mycluster, mynow_filename, mylfn_filename);
        mycluster = myfat_table[mycluster] & 0x0FFFFFFF;
    }
    return 0;
}

int mylistallfunc(const string &myimg){
    string mylfn_filename = "";
    int myfd = open(myimg.c_str(), O_RDONLY | O_NONBLOCK);
    if(myfd <= 0){
        perror("open failed");
        return -1;
    }
    if(myread_BS(myfd) < 0){
        return -1;
    }
    if(myread_fat(myfd) < 0){
        return -1;
    }
    myread_cluster_chain(myfd, myroot_cluster_32, "./");
    close(myfd);
    return 0;
}

int main(int argc, char **argv){
    if(argc < 2){
        printf("USAGE: ./listall DEVICE");
        return 0;
    }
    mylistallfunc(argv[1]);
    return 0;
}
