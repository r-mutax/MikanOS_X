#include <cstdio>
#include <cstdlib>

extern "C" void main(int argc, char** argv) {
    if (argc < 3){
        printf("Usage %s <src> <dest>\n");
        exit(1);
    }

    FILE* fp_src = fopen(argv[1], "r");
    if(fp_src == nullptr){
        printf("failed to open for read: %s\n");
        exit(1);
    }

    FILE* fp_dst = fopen(argv[2], "w");
    if(fp_dst == nullptr){
        printf("failed to open for write: %s\n");
        exit(1);
    }

    char buf[256];
    size_t bytes;
    while((bytes = fread(buf, 1, sizeof(buf), fp_src)) > 0){
        const size_t written = fwrite(buf, 1, bytes, fp_dst);
        if(bytes != written){
            printf("failed to write to %s\n", argv[2]);
            exit(1);
        }
    }
    exit(0);
}