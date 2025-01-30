#define FUSE_USE_VERSION 26

#include <fuse.h>
#include "../libWad/Wad.h"
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sys/stat.h>

#define WAD_PTR ((Wad*)fuse_get_context()->private_data)

static int wad_getattr(const char *path, struct stat *stats){
	stats->st_uid = getuid();
	stats->st_gid = getgid();
	stats->st_atime = time(NULL);
	stats->st_mtime = time(NULL);

    if (WAD_PTR->isDirectory(path)){
		stats->st_mode = S_IFDIR | 0777;
		//stats->st_mode = S_IFDIR | 0555;
		stats->st_nlink = 2;
		return 0;
	}
	else if(((Wad*)(fuse_get_context()->private_data))->isContent(path)){
		stats->st_mode = S_IFREG | 0777;
		//stats->st_mode = S_IFREG | 0444;
		stats->st_nlink = 1;
		stats->st_size = ((Wad*)(fuse_get_context()->private_data))->getSize(path);
		return 0;
	}
	return -ENOENT;
}

static int wad_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    std::vector<std::string> directoryFiles;
    WAD_PTR->getDirectory(path, &directoryFiles);
    filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
    for(std::string entry : directoryFiles){
        filler(buffer, entry.c_str(), NULL, 0);
    }
    return 0;
}

static int wad_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	return WAD_PTR->getContents(path, buffer, size, offset);
}

static int wad_mkdir(const char* path, mode_t mode){
	WAD_PTR->createDirectory(path);
	return 0;
}

static int wad_mknod(const char *path, mode_t mode, dev_t rdev){
	WAD_PTR->createFile(path);
	return 0;
}

static int wad_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info){
	return WAD_PTR->writeToFile(path, buffer, size, offset);
}

static void wad_destroy(void* private_data){    
	delete ((Wad*)private_data);
}

static struct fuse_operations operations = {
	.getattr = wad_getattr,
	.mknod = wad_mknod,
	.mkdir = wad_mkdir,
	.read = wad_read,
	.write = wad_write,
	.readdir = wad_readdir,
	.destroy = wad_destroy,
};

int main(int argc, char* argv[]){
    if(argc < 3) {	
		std::cout << "Not enough arguments." << std::endl;
        exit(EXIT_SUCCESS);
    }
    
    std::string workingDirectory = get_current_dir_name(); 
    std::string wadPath = argv[argc - 2];
    
    if(wadPath.at(0) != '/'){
	    if(wadPath.at(0) == '.'){
			if(wadPath.at(1) == '.'){
				std::cout << "Vertical relative \"..\" directories not allowed. Exiting." << std::endl;
        		exit(EXIT_SUCCESS);
			}
		    wadPath = wadPath.substr(1, wadPath.length() - 1);
	    }
	    wadPath = workingDirectory + "/" + wadPath;
    }
    perror(wadPath.c_str());
    Wad* myWad = Wad::loadWad(wadPath);

    argv[argc - 2] = argv[argc - 1];
    argc--;
    return fuse_main(argc, argv, &operations, myWad);
}
