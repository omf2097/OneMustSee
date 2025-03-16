#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>

typedef enum {
    UINT8 = 0,
    INT8,
    UINT16,
    INT16,
    UINT32,
    INT32,
    PSEUDOFLOAT,
    POINTER
} DataType;

typedef enum {
    // read an address relative to the anchor - anchor_offset
    OFFSET_DIRECT,
    // read a pointer, then read the memory that pointer points at +offset
    OFFSET_POINTER,
    // read absolute memory addresses, probably not very useful
    OFFSET_ABSOLUTE
} OffsetMode;

typedef struct {
    OffsetMode mode;
    union {
        long direct;
        struct {
            long ptr_offset;
            long data_offset; // TODO this could be a list of offsets for pointers to pointers
        } pointer;
        uintptr_t absolute;
    } offset;
    char *name;
    DataType type;
} WatchOffset;

typedef struct {
    OffsetMode mode;
    uintptr_t address;
    long base;
    long offset;
    long pointer_offset;
    int size;
    DataType type;
    char *name;
    unsigned char *last_value;
} WatchEntry;

typedef struct {
    uintptr_t start;
    uintptr_t end;
    int is_file_backed;  // 1 if mapped from a file, 0 for anonymous
    int is_special;
} MemoryRegion;

typedef struct {
    const char *anchor;
    size_t anchor_len;
    uint64_t anchor_offset;
    struct {
        long value;
        long pointer;
    } pointer_offset;
    WatchOffset *watch_offsets;
    int num_watches;
} AnchorConfig;

// Anchor and pointer configuration
AnchorConfig anchor_configs[] = {
    {
        .anchor = "l20s4sp13zzN3-zzM100",
        .anchor_len = 21,
        // how far this string is into the memory space
        .anchor_offset = 0x233068,
        // we know that the HAR1 pointer is at 0x237970 and we expect it to point at 0x255038
        // if that's not the case, we will calculate the correct offset to use when reading pointers
        .pointer_offset = {0x255038, 0x237970},
        .watch_offsets = (WatchOffset[]){
            {OFFSET_DIRECT, {.direct = 0x23eb64}, "tick", UINT16},
            {OFFSET_POINTER, {.pointer = {0x237978, 0x108}}, "P1 HP", INT16},
            //{OFFSET_POINTER, {.pointer = {0x237978, 0x10A}}, "P1 max HP", INT16},
            //{OFFSET_POINTER, {.pointer = {0x237970, 0x30}}, "P1 Har ID", UINT8},
            {OFFSET_POINTER, {.pointer = {0x237970, 0x4c}}, "P1 Har animation", UINT8},
            //{OFFSET_POINTER, {.pointer = {0x237970, 0x88}}, "P1 stun", INT32},
            {OFFSET_POINTER, {.pointer = {0x237970, 0xA8}}, "P1 XPos", PSEUDOFLOAT},
            {OFFSET_POINTER, {.pointer = {0x237970, 0xAC}}, "P1 YPos", PSEUDOFLOAT},
            {OFFSET_POINTER, {.pointer = {0x237970, 0xb4}}, "P1 XVel", PSEUDOFLOAT},
            {OFFSET_POINTER, {.pointer = {0x237970, 0xb0}}, "P1 YVel", PSEUDOFLOAT},

            {OFFSET_POINTER, {.pointer = {0x23797c, 0x108}}, "P2 HP", INT16},
            //{OFFSET_POINTER, {.pointer = {0x23797c, 0x10A}}, "P2 max HP", INT16},
            //{OFFSET_POINTER, {.pointer = {0x237974, 0x30}}, "P2 Har ID", UINT8},
            {OFFSET_POINTER, {.pointer = {0x237974, 0x4c}}, "P2 Har animation", UINT8},
            //{OFFSET_POINTER, {.pointer = {0x237974, 0x88}}, "P2 stun", INT32},
            {OFFSET_POINTER, {.pointer = {0x237974, 0xA8}}, "P2 XPos", PSEUDOFLOAT},
            {OFFSET_POINTER, {.pointer = {0x237974, 0xAC}}, "P2 YPos", PSEUDOFLOAT},
            {OFFSET_POINTER, {.pointer = {0x237974, 0xb4}}, "P2 XVel", PSEUDOFLOAT},
            {OFFSET_POINTER, {.pointer = {0x237974, 0xb0}}, "P2 YVel", PSEUDOFLOAT},

            //{OFFSET_DIRECT, {.direct = 0x255068}, "HAR 1 ID", UINT8},
            //{OFFSET_DIRECT, {.direct = 0x255084}, "HAR 1 anim", UINT8},
            //{OFFSET_DIRECT, {.direct = 139384}, "HAR 1 Xpos", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 139388}, "HAR 1 Ypos", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 139392}, "HAR 1 XVel", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 139396}, "HAR 1 YVel", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 0x255130}, "HAR 1 enemy pointer", POINTER},
            //{OFFSET_DIRECT, {.direct = 0x260068}, "HAR 2 ID", UINT8},
            //{OFFSET_DIRECT, {.direct = 184348}, "HAR 2 anim", UINT8},
            //{OFFSET_DIRECT, {.direct = 184440}, "HAR 2 Xpos", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 184444}, "HAR 2 Ypos", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 184448}, "HAR 2 XVel", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 184452}, "HAR 2 YVel", PSEUDOFLOAT},
            //{OFFSET_DIRECT, {.direct = 0x260130}, "HAR 2 enemy pointer", POINTER},

            //{OFFSET_DIRECT, {.absolute = 18704}, "P1 pilot", POINTER}

        },
        .num_watches = 13
    },
    /*
     // example of finding a pointer (this is how I located the HAR pointers)
     {
      //.anchor = "\x38\x50\x25\x00",
      .anchor = "\xa0\x80\x48\x00",
      .anchor_len = 4,
      .watch_offsets = (WatchOffset[]){
      {OFFSET_DIRECT, {.direct = 0x108}, "P1 health", UINT16},
      {OFFSET_DIRECT, {.direct = 0x10A}, "P1 max health", UINT16},
      },
      .num_watches = 2
      },
      // example of finding a pilot by their name
      {
      .anchor = "Crystal",
      .anchor_len = 7,
      .watch_offsets = (WatchOffset[]){
      {OFFSET_DIRECT, {.direct = 0x104}, "P1 health", UINT16},
      {OFFSET_DIRECT, {.direct = 0x106}, "P1 max health", UINT16},
      },
      .num_watches = 2
      },
      {
      .anchor = "Shirro",
      .anchor_len = 6,
      .watch_offsets = (WatchOffset[]){
      {OFFSET_DIRECT, {.direct = 0x104}, "P2 health", UINT16},
      {OFFSET_DIRECT, {.direct = 0x106}, "P2 max health", UINT16},
      },
      .num_watches = 2
      }*/
};
const int num_anchors = sizeof(anchor_configs) / sizeof(AnchorConfig);

// Helper functions
void print_value(DataType type, unsigned char *data) {
    switch (type) {
        case UINT8: printf("%u", *(uint8_t *)data); break;
        case INT8: printf("%d", *(int8_t *)data); break;
        case UINT16: printf("%u", *(uint16_t *)data); break;
        case INT16: printf("%d", *(int16_t *)data); break;
        case UINT32: printf("%" PRIu32, *(uint32_t *)data); break;
        case INT32: printf("%" PRId32, *(int32_t *)data); break;
        case PSEUDOFLOAT: printf("%f", (*(int32_t *)data) / 256.0f); break;
        case POINTER: printf("0x%" PRIx32, *(int32_t *)data); break;
        default: printf("<?>");
    }
}

static char* trim_whitespace(char *str) {
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
    return str;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pid> [--csv]\n", argv[0]);
        return 1;
    }
    pid_t pid = atoi(argv[1]);

    if(!pid) {
        fprintf(stderr, "Usage: %s <pid> [--csv]\n", argv[0]);
        return 1;
    }

    int csv = 0;
    if(argc > 2 && strcmp(argv[2], "--csv") == 0) {
        csv = 1;
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s <pid> [--csv]\n", argv[0]);
        return 1;
    }

    // Parse memory maps
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    FILE *maps_file = fopen(maps_path, "r");
    if (!maps_file) {
        perror("Failed to open maps file");
        return 1;
    }

    MemoryRegion *regions = NULL;
    int num_regions = 0;
    char line[256];
    while (fgets(line, sizeof(line), maps_file)) {
        uintptr_t start, end;
        char perms[5];
        unsigned long offset;
        unsigned int dev_major, dev_minor;
        unsigned long inode;
        char pathname[256] = {0};

        // Improved parsing with whitespace handling
        int matches = sscanf(line, "%lx-%lx %4s %lx %x:%x %lu %255[^\n]",
                &start, &end, perms, &offset,
                &dev_major, &dev_minor, &inode, pathname);

        // Trim whitespace from pathname
        char *clean_path = trim_whitespace(pathname);

        // Classify memory regions more accurately
        int is_file_backed = 0;
        int is_special_region = 0;

        if (matches >= 7) {  // At least got through inode
                             // Check for special regions
            is_special_region = (strcmp(clean_path, "[heap]") == 0) ||
                (strcmp(clean_path, "[stack]") == 0) ||
                (strcmp(clean_path, "[vdso]") == 0) ||
                (strncmp(clean_path, "[anon:", 6) == 0);

            // Consider file-backed if:
            // - Has non-empty pathname
            // - Not a special region
            // - Offset is not zero (for non-anonymous mappings)
            is_file_backed = (clean_path[0] != '\0' &&
                    !is_special_region &&
                    (offset != 0 || inode != 0));
        }

        // Only consider readable regions
        if (strchr(perms, 'r')) {
            regions = realloc(regions, (num_regions + 1) * sizeof(MemoryRegion));
            regions[num_regions] = (MemoryRegion){
                .start = start,
                    .end = end,
                    .is_file_backed = is_file_backed,
                    .is_special = is_special_region
            };
            num_regions++;
        }
    }
    fclose(maps_file);

    // Open memory file descriptor
    char mem_path[256];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    int mem_fd = open(mem_path, O_RDONLY);
    if (mem_fd == -1) {
        perror("Failed to open mem file");
        free(regions);
        return 1;
    }

    // Process all anchor configurations
    WatchEntry *entries = NULL;
    int num_entries = 0;

    for (int cfg_idx = 0; cfg_idx < num_anchors; cfg_idx++) {
        AnchorConfig *cfg = &anchor_configs[cfg_idx];
        uintptr_t *anchor_addrs = NULL;
        long *offsets = NULL;
        int num_found = 0;

        // Search for current anchor
        for (int i = 0; i < num_regions; i++) {
            // Skip file-backed regions (shared libraries, executables, etc)
            if (regions[i].is_file_backed) {
                continue;
            }
            size_t size = regions[i].end - regions[i].start;
            unsigned char *buffer = malloc(size);

            if (pread(mem_fd, buffer, size, regions[i].start) != (ssize_t)size) {
                free(buffer);
                continue;
            }

            for (size_t off = 0; off <= size - cfg->anchor_len; off++) {
                if (memcmp(buffer + off, cfg->anchor, cfg->anchor_len) == 0) {

                    if(!csv) {
                        printf("Anchor %s found at 0x%lx in region %lx-%lx\n", cfg->anchor, regions[i].start + off - cfg->anchor_offset, regions[i].start, regions[i].end);
                    }
                    anchor_addrs = realloc(anchor_addrs, (num_found + 1) * sizeof(uintptr_t));
                    anchor_addrs[num_found++] = regions[i].start + off - cfg->anchor_offset;
                    
                    long offset = 0;

                    if (cfg->anchor_offset && cfg->pointer_offset.pointer) {
                        pread(mem_fd, buffer, 4, regions[i].start + off - cfg->anchor_offset + cfg->pointer_offset.pointer);
                        int pointer_value = *(int32_t *)buffer;
                        if(!csv) {
                            printf("ptr value 0x%lx , expected value 0x%lx\n", pointer_value, cfg->pointer_offset.value);
                        }
                        if(pointer_value != cfg->pointer_offset.value) {
                            offset = pointer_value - cfg->pointer_offset.value;
                            if(!csv) {
                                printf("computed pointer offset of %d\n", offset);
                            }
                        }
                    }

                    offsets = realloc(offsets, (num_found + 1) * sizeof(long));
                    offsets[num_found++] = offset;
                }
            }
            free(buffer);
        }

        if (num_found == 0) {
            fprintf(stderr, "Anchor '%s' not found\n", cfg->anchor);
            continue;
        }

        // Process found anchors
        for (int i = 0; i < num_found; i++) {
            uintptr_t base = anchor_addrs[i];

            for (int j = 0; j < cfg->num_watches; j++) {
                WatchOffset *wo = &cfg->watch_offsets[j];
                uintptr_t addr;
                int size;
                long offset = 0;

                // Calculate address based on offset type
                if (wo->mode == OFFSET_DIRECT) {
                    addr = base + wo->offset.direct;
                } else if (wo->mode == OFFSET_POINTER) {
                    uint32_t ptr_value;
                    uintptr_t ptr_addr = base + wo->offset.pointer.ptr_offset;


                    if (pread(mem_fd, &ptr_value, sizeof(ptr_value), ptr_addr) != sizeof(ptr_value))
                        continue;

                    if(!csv) {
                        printf("read pointer value 0x%lx for %s pointed to 0x%lx\n", ptr_addr - base, wo->name, ptr_value);
                    }

                    addr = base + wo->offset.pointer.ptr_offset;
                    offset = wo->offset.pointer.data_offset;
                } else { // OFFSET_ABSOLUTE
                    addr = wo->offset.absolute;
                }

                // Determine size from data type
                switch (wo->type) {
                    case UINT8: case INT8: size = 1; break;
                    case UINT16: case INT16: size = 2; break;
                    case UINT32: case INT32: case PSEUDOFLOAT: case POINTER: size = 4; break;
                    default: continue;
                }

                // Validate address
                int valid = 0;
                for (int k = 0; k < num_regions; k++) {
                    if (addr >= regions[k].start && addr + size <= regions[k].end) {
                        valid = 1;
                        break;
                    }
                }
                if (!valid) {
                    continue; 
                }

                // Read initial value
                unsigned char *buf = malloc(size);
                if (pread(mem_fd, buf, size, addr) != size) {
                    printf("unable to read field %s\n", wo->name);
                    free(buf);
                    continue;
                }

                // Create entry
                entries = realloc(entries, (num_entries + 1) * sizeof(WatchEntry));
                entries[num_entries] = (WatchEntry){
                    .address = addr,
                        .offset = offset,
                        .pointer_offset = offsets[i],
                        .base = base,
                        .size = size,
                        .type = wo->type,
                        .mode = wo->mode,
                        .name = strdup(wo->name),
                        .last_value = buf
                };
                num_entries++;
            }
        }
        free(anchor_addrs);
        free(offsets);
    }

    free(regions);

    if (num_entries == 0) {
        fprintf(stderr, "No valid watch entries\n");
        close(mem_fd);
        return 1;
    }

    // Monitoring loop with initial print
    if(!csv) {
        printf("Monitoring %d values...\n", num_entries);
    }
    unsigned char *current = malloc(4);
    int first_iteration = 1;

    while (1) {
        int changed = 0;
        for (int i = 0; i < num_entries; i++) {
            WatchEntry *e = &entries[i];
            ssize_t bytes_read; 
            uintptr_t addr;
            if(e->mode == OFFSET_POINTER) {
                bytes_read = pread(mem_fd, current, 4, e->address);
                if (bytes_read != 4) {
                    printf("[%s @ 0x%lx] Pointer Read error\n", e->name, e->address - e->base);
                }
                uint32_t ptr_addr = (*(uint32_t *)current);
                //printf("[%s @ 0x%lx] Pointer pointed at 0x%lx\n", e->name, e->address - e->base, ptr_addr + e->offset);
                // XXX I don't know why we subtract the offset, but it works?!
                bytes_read = pread(mem_fd, current, e->size, ptr_addr + e->base + e->offset - e->pointer_offset);
                addr = ptr_addr + e->offset;
            } else {
                addr = e->address - e->base;
                bytes_read = pread(mem_fd, current, e->size, e->address);
            }

            if (bytes_read != e->size) {
                if (first_iteration) {
                    printf("[%s @ 0x%lx] Read error\n", e->name, addr);
                }
                continue;
            }

            if(csv) {
                // tick changed, dump all the last values
                if (memcmp(current, e->last_value, e->size) != 0 && i == 0) {
                    changed = 1;
                    if(csv) {
                        for (int i = 0; i < num_entries; i++) {
                            WatchEntry *e = &entries[i];
                            print_value(e->type, e->last_value);
                            if(i == num_entries -1) {
                                printf("\n");
                            } else {
                                printf(",");
                            }
                        }

                    }

                }

            } else {
                if (first_iteration) {
                    printf("[%s @ 0x%lx] Initial: ", e->name, addr);
                    print_value(e->type, current);
                    printf("\n");
                } else {
                    if (memcmp(current, e->last_value, e->size) != 0) {
                        printf("[%s @ 0x%lx] Changed: ", e->name, addr);
                        printf("Old: ");
                        print_value(e->type, e->last_value);
                        printf(" → New: ");
                        print_value(e->type, current);
                        printf("\n");
                    }
                }
            }

            memcpy(e->last_value, current, e->size);
        }


        if (first_iteration) {
            first_iteration = 0;
            if(csv) {
                for (int i = 0; i < num_entries; i++) {
                    WatchEntry *e = &entries[i];
                    if(i == num_entries -1) {
                        printf("%s\n", e->name);
                    } else {
                        printf("%s,", e->name);
                    }
                }
            } else {
                printf("\nStarting continuous monitoring...\n\n");
            }
        }
        usleep(1);
    }

    close(mem_fd);
    free(current);
    for (int i = 0; i < num_entries; i++) {
        free(entries[i].name);
        free(entries[i].last_value);
    }
    free(entries);
    return 0;
}
