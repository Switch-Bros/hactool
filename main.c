#include <getopt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "utils.h"
#include "settings.h"
#include "pki.h"
#include "nca.h"
#include "xci.h"
#include "extkeys.h"
#include "packages.h"

static char *prog_name = "hactool";

/* Print usage. Taken largely from ctrtool. */
static void usage(void) {
    fprintf(stderr, 
        "hactool (c) SciresM.\n"
        "Built: %s %s\n"
        "\n"
        "Usage: %s [options...] <file>\n"
        "Options:\n"
        "-i, --info        Show file info.\n"
        "                      This is the default action.\n"
        "-x, --extract     Extract data from file.\n"
        "                      This is also the default action.\n"
        "  -r, --raw          Keep raw data, don't unpack.\n"
        "  -y, --verify       Verify hashes and signatures.\n"
        "  -d, --dev          Decrypt with development keys instead of retail.\n"
        "  -k, --keyset       Load keys from an external file.\n"
        "  -t, --intype=type  Specify input file type [nca, xci, pfs0, romfs, hfs0, npdm, pk11]\n"
        "  --titlekey=key     Set title key for Rights ID crypto titles.\n"
        "  --contentkey=key   Set raw key for NCA body decryption.\n"
        "NCA options:\n"
        "  --plaintext=file   Specify file path for saving a decrypted copy of the NCA.\n"
        "  --header=file      Specify Header file path.\n"
        "  --section0=file    Specify Section 0 file path.\n"
        "  --section1=file    Specify Section 1 file path.\n"
        "  --section2=file    Specify Section 2 file path.\n"
        "  --section3=file    Specify Section 3 file path.\n"
        "  --section0dir=dir  Specify Section 0 directory path.\n"
        "  --section1dir=dir  Specify Section 1 directory path.\n"
        "  --section2dir=dir  Specify Section 2 directory path.\n"
        "  --section3dir=dir  Specify Section 3 directory path.\n"
        "  --exefs=file       Specify ExeFS file path. Overrides appropriate section file path.\n"
        "  --exefsdir=dir     Specify ExeFS directory path. Overrides appropriate section directory path.\n"
        "  --romfs=file       Specify RomFS file path. Overrides appropriate section file path.\n"
        "  --romfsdir=dir     Specify RomFS directory path. Overrides appropriate section directory path.\n"
        "  --listromfs        List files in RomFS.\n"
        "  --baseromfs        Set Base RomFS to use with update partitions.\n"
        "  --basenca          Set Base NCA to use with update partitions.\n" 
        "PFS0 options:\n"
        "  --pfs0dir=dir      Specify PFS0 directory path.\n"
        "  --outdir=dir       Specify PFS0 directory path. Overrides previous path, if present.\n"
        "  --exefsdir=dir     Specify PFS0 directory path. Overrides previous paths, if present for ExeFS PFS0.\n"
        "RomFS options:\n"
        "  --romfsdir=dir     Specify RomFS directory path.\n"
        "  --outdir=dir       Specify RomFS directory path. Overrides previous path, if present.\n"
        "  --listromfs        List files in RomFS.\n"
        "HFS0 options:\n"
        "  --hfs0dir=dir      Specify HFS0 directory path.\n"
        "  --outdir=dir       Specify HFS0 directory path. Overrides previous path, if present.\n"
        "  --exefsdir=dir     Specify HFS0 directory path. Overrides previous paths, if present.\n"
        "XCI options:\n"
        "  --rootdir=dir      Specify XCI root HFS0 directory path.\n"
        "  --updatedir=dir    Specify XCI update HFS0 directory path.\n"
        "  --normaldir=dir    Specify XCI normal HFS0 directory path.\n"
        "  --securedir=dir    Specify XCI secure HFS0 directory path.\n"
        "  --outdir=dir       Specify XCI directory path. Overrides previous paths, if present.\n"
        "Package1 options:\n"
        "  --package1dir=dir  Specify Package1 directory path.\n"
        "  --outdir=dir       Specify Package1 directory path. Overrides previous path, if present.\n"
        "\n", __TIME__, __DATE__, prog_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    hactool_ctx_t tool_ctx;
    hactool_ctx_t base_ctx; /* Context for base NCA, if used. */
    nca_ctx_t nca_ctx;
    char input_name[0x200];
    filepath_t keypath;
    
    prog_name = (argc < 1) ? "hactool" : argv[0];

    nca_init(&nca_ctx);
    memset(&tool_ctx, 0, sizeof(tool_ctx));
    memset(&base_ctx, 0, sizeof(base_ctx));
    memset(input_name, 0, sizeof(input_name));
    filepath_init(&keypath);
    nca_ctx.tool_ctx = &tool_ctx;
    
    nca_ctx.tool_ctx->file_type = FILETYPE_NCA;
    base_ctx.file_type = FILETYPE_NCA;

    nca_ctx.tool_ctx->action = ACTION_INFO | ACTION_EXTRACT;
    pki_initialize_keyset(&tool_ctx.settings.keyset, KEYSET_RETAIL);

    while (1) {
        int option_index;
        char c;
        static struct option long_options[] = 
        {
            {"extract", 0, NULL, 'x'},
            {"info", 0, NULL, 'i'},
            {"dev", 0, NULL, 'd'},
            {"verify", 0, NULL, 'y'},
            {"raw", 0, NULL, 'r'},
            {"intype", 1, NULL, 't'},
            {"keyset", 1, NULL, 'k'},
            {"section0", 1, NULL, 0},
            {"section1", 1, NULL, 1},
            {"section2", 1, NULL, 2},
            {"section3", 1, NULL, 3},
            {"section0dir", 1, NULL, 4},
            {"section1dir", 1, NULL, 5},
            {"section2dir", 1, NULL, 6},
            {"section3dir", 1, NULL, 7},
            {"exefs", 1, NULL, 8},
            {"romfs", 1, NULL, 9},
            {"exefsdir", 1, NULL, 10},
            {"romfsdir", 1, NULL, 11},
            {"titlekey", 1, NULL, 12},
            {"contentkey", 1, NULL, 13},
            {"listromfs", 0, NULL, 14},
            {"baseromfs", 1, NULL, 15},
            {"basenca", 1, NULL, 16},
            {"outdir", 1, NULL, 17},
            {"plaintext", 1, NULL, 18},
            {"header", 1, NULL, 19},
            {"pfs0dir", 1, NULL, 20},
            {"hfs0dir", 1, NULL, 21},
            {"rootdir", 1, NULL, 22},
            {"updatedir", 1, NULL, 23},
            {"normaldir", 1, NULL, 24},
            {"securedir", 1, NULL, 25},
            {"package1dir", 1, NULL, 26},
            {NULL, 0, NULL, 0},
        };

        c = getopt_long(argc, argv, "dryxt:ik:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) 
        {
            case 'i':
                nca_ctx.tool_ctx->action |= ACTION_INFO;
                break;
            case 'x':
                nca_ctx.tool_ctx->action |= ACTION_EXTRACT;
                break;
            case 'y':
                nca_ctx.tool_ctx->action |= ACTION_VERIFY;
                break;
            case 'r':
                nca_ctx.tool_ctx->action |= ACTION_RAW;
                break;
            case 'd':
                pki_initialize_keyset(&tool_ctx.settings.keyset, KEYSET_DEV);
                nca_ctx.tool_ctx->action |= ACTION_DEV;
                break;
            case 'k':
                filepath_set(&keypath, optarg);
                break;
            case 't':
                if (!strcmp(optarg, "nca")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_NCA;
                } else if (!strcmp(optarg, "pfs0") || !strcmp(optarg, "exefs")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_PFS0;
                } else if (!strcmp(optarg, "romfs")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_ROMFS; 
                } else if (!strcmp(optarg, "hfs0")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_HFS0;
                } else if (!strcmp(optarg, "xci") || !strcmp(optarg, "gamecard") || !strcmp(optarg, "gc")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_XCI;
                } else if (!strcmp(optarg, "npdm") || !strcmp(optarg, "meta")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_NPDM;
                } else if (!strcmp(optarg, "package1") || !strcmp(optarg, "pk11")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_PACKAGE1;
                } else if (!strcmp(optarg, "package2") || !strcmp(optarg, "pk21")) {
                    nca_ctx.tool_ctx->file_type = FILETYPE_PACKAGE2;
                }
                break;
            case 0: filepath_set(&nca_ctx.tool_ctx->settings.section_paths[0], optarg); break;
            case 1: filepath_set(&nca_ctx.tool_ctx->settings.section_paths[1], optarg); break;
            case 2: filepath_set(&nca_ctx.tool_ctx->settings.section_paths[2], optarg); break;
            case 3: filepath_set(&nca_ctx.tool_ctx->settings.section_paths[3], optarg); break;
            case 4: filepath_set(&nca_ctx.tool_ctx->settings.section_dir_paths[0], optarg); break;
            case 5: filepath_set(&nca_ctx.tool_ctx->settings.section_dir_paths[1], optarg); break;
            case 6: filepath_set(&nca_ctx.tool_ctx->settings.section_dir_paths[2], optarg); break;
            case 7: filepath_set(&nca_ctx.tool_ctx->settings.section_dir_paths[3], optarg); break;
            case 8:
                nca_ctx.tool_ctx->settings.exefs_path.enabled = 1;
                filepath_set(&nca_ctx.tool_ctx->settings.exefs_path.path, optarg); 
                break;
            case 9:
                nca_ctx.tool_ctx->settings.romfs_path.enabled = 1;
                filepath_set(&nca_ctx.tool_ctx->settings.romfs_path.path, optarg); 
                break;
            case 10:
                nca_ctx.tool_ctx->settings.exefs_dir_path.enabled = 1;
                filepath_set(&nca_ctx.tool_ctx->settings.exefs_dir_path.path, optarg); 
                break;
            case 11:
                nca_ctx.tool_ctx->settings.romfs_dir_path.enabled = 1;
                filepath_set(&nca_ctx.tool_ctx->settings.romfs_dir_path.path, optarg); 
                break;
            case 12:
                parse_hex_key(nca_ctx.tool_ctx->settings.titlekey, optarg, 16);
                nca_ctx.tool_ctx->settings.has_titlekey = 1;
                break;
            case 13:
                parse_hex_key(nca_ctx.tool_ctx->settings.contentkey, optarg, 16);
                nca_ctx.tool_ctx->settings.has_contentkey = 1;
                break;
            case 14:
                nca_ctx.tool_ctx->action |= ACTION_LISTROMFS;
                break;
            case 15:
                if (nca_ctx.tool_ctx->base_file != NULL) {
                    usage();
                    return EXIT_FAILURE;
                }
                if ((nca_ctx.tool_ctx->base_file = fopen(optarg, "rb")) == NULL) {
                    fprintf(stderr, "unable to open %s: %s\n", optarg, strerror(errno));
                    return EXIT_FAILURE;
                }
                nca_ctx.tool_ctx->base_file_type = BASEFILE_ROMFS;
                break;
            case 16:
                if (nca_ctx.tool_ctx->base_file != NULL) {
                    usage();
                    return EXIT_FAILURE;
                }
                if ((nca_ctx.tool_ctx->base_file = fopen(optarg, "rb")) == NULL) {
                    fprintf(stderr, "unable to open %s: %s\n", optarg, strerror(errno));
                    return EXIT_FAILURE;
                }
                nca_ctx.tool_ctx->base_file_type = BASEFILE_NCA;
                nca_ctx.tool_ctx->base_nca_ctx = malloc(sizeof(*nca_ctx.tool_ctx->base_nca_ctx));
                if (nca_ctx.tool_ctx->base_nca_ctx == NULL) {
                    fprintf(stderr, "Failed to allocate base NCA context!\n");
                    return EXIT_FAILURE;
                }
                nca_init(nca_ctx.tool_ctx->base_nca_ctx);
                base_ctx.file = nca_ctx.tool_ctx->base_file;
                nca_ctx.tool_ctx->base_nca_ctx->file = base_ctx.file;
                break;
            case 17:
                tool_ctx.settings.out_dir_path.enabled = 1;
                filepath_set(&tool_ctx.settings.out_dir_path.path, optarg); 
                break;
            case 18:
                filepath_set(&nca_ctx.tool_ctx->settings.dec_nca_path, optarg); 
                break;
            case 19:
                filepath_set(&nca_ctx.tool_ctx->settings.header_path, optarg); 
                break;
            case 20:
                filepath_set(&tool_ctx.settings.pfs0_dir_path, optarg); 
                break;
            case 21:
                filepath_set(&tool_ctx.settings.hfs0_dir_path, optarg); 
                break;
            case 22:
                filepath_set(&tool_ctx.settings.rootpt_dir_path, optarg); 
                break;
            case 23:
                filepath_set(&tool_ctx.settings.update_dir_path, optarg); 
                break;
            case 24:
                filepath_set(&tool_ctx.settings.normal_dir_path, optarg); 
                break;
            case 25:
                filepath_set(&tool_ctx.settings.secure_dir_path, optarg); 
                break;
            case 26:
                filepath_set(&tool_ctx.settings.pk11_dir_path, optarg); 
                break;
            default:
                usage();
                return EXIT_FAILURE;
        }
    }
    
    /* Try to populate default keyfile. */
    if (keypath.valid == VALIDITY_INVALID) {
        char *home = getenv("HOME");
        if (home == NULL) {
            home = getenv("USERPROFILE");
        }
        if (home != NULL) {
            filepath_set(&keypath, home);
            filepath_append(&keypath, ".switch");
            filepath_append(&keypath, "%s.keys", (tool_ctx.action & ACTION_DEV) ? "dev" : "prod");
        }
    }
    
    /* Load external keys, if relevant. */
    if (keypath.valid == VALIDITY_VALID) {
        FILE *keyfile = os_fopen(keypath.os_path, OS_MODE_READ);
        if (keyfile != NULL) {
            extkeys_initialize_keyset(&tool_ctx.settings.keyset, keyfile);
            pki_derive_keys(&tool_ctx.settings.keyset);
            fclose(keyfile);
        }
    }
    

    

    if (optind == argc - 1) {
        /* Copy input file. */
        strncpy(input_name, argv[optind], sizeof(input_name));
    } else if ((optind < argc) || (argc == 1)) {
        usage();
    }

    if ((tool_ctx.file = fopen(input_name, "rb")) == NULL) {
        fprintf(stderr, "unable to open %s: %s\n", input_name, strerror(errno));
        return EXIT_FAILURE;
    }
    
    switch (tool_ctx.file_type) {
        case FILETYPE_NCA: {
            if (nca_ctx.tool_ctx->base_nca_ctx != NULL) {
                memcpy(&base_ctx.settings.keyset, &tool_ctx.settings.keyset, sizeof(nca_keyset_t));
                nca_ctx.tool_ctx->base_nca_ctx->tool_ctx = &base_ctx;
                nca_process(nca_ctx.tool_ctx->base_nca_ctx);
                int found_romfs = 0;
                for (unsigned int i = 0; i < 4; i++) {
                    if (nca_ctx.tool_ctx->base_nca_ctx->section_contexts[i].is_present && nca_ctx.tool_ctx->base_nca_ctx->section_contexts[i].type == ROMFS) {
                        found_romfs = 1;
                        break;
                    }
                }
                if (found_romfs == 0) {
                    fprintf(stderr, "Unable to locate RomFS in base NCA!\n");
                    return EXIT_FAILURE;
                }
            }

            nca_ctx.file = tool_ctx.file;
            nca_process(&nca_ctx);
            nca_free_section_contexts(&nca_ctx);
            
            if (nca_ctx.tool_ctx->base_file != NULL) {
                fclose(nca_ctx.tool_ctx->base_file);
                if (nca_ctx.tool_ctx->base_file_type == BASEFILE_NCA) {
                    nca_free_section_contexts(nca_ctx.tool_ctx->base_nca_ctx);
                    free(nca_ctx.tool_ctx->base_nca_ctx);
                }
            }     
            break;
        }
        case FILETYPE_PFS0: {
            pfs0_ctx_t pfs0_ctx;
            memset(&pfs0_ctx, 0, sizeof(pfs0_ctx));
            pfs0_ctx.file = tool_ctx.file;
            pfs0_ctx.tool_ctx = &tool_ctx;
            pfs0_process(&pfs0_ctx);
            if (pfs0_ctx.header) {
                free(pfs0_ctx.header);
            }
            if (pfs0_ctx.npdm) {
                free(pfs0_ctx.npdm);
            }
            break;
        }
        case FILETYPE_ROMFS: {
            romfs_ctx_t romfs_ctx;
            memset(&romfs_ctx, 0, sizeof(romfs_ctx));
            romfs_ctx.file = tool_ctx.file;
            romfs_ctx.tool_ctx = &tool_ctx;
            romfs_process(&romfs_ctx);
            if (romfs_ctx.files) {
                free(romfs_ctx.files);
            }
            if (romfs_ctx.directories) {
                free(romfs_ctx.directories);
            }
            break;
        }
        case FILETYPE_NPDM: {
            npdm_t raw_hdr;
            memset(&raw_hdr, 0, sizeof(raw_hdr));
            if (fread(&raw_hdr, 1, sizeof(raw_hdr), tool_ctx.file) != sizeof(raw_hdr)) {
                fprintf(stderr, "Failed to read NPDM header!\n");
                exit(EXIT_FAILURE);
            }
            if (raw_hdr.magic != MAGIC_META) {
                fprintf(stderr, "NPDM seems corrupt!\n");
                exit(EXIT_FAILURE);
            }
            uint64_t npdm_size = raw_hdr.aci0_size + raw_hdr.aci0_offset;
            if (raw_hdr.acid_offset + raw_hdr.acid_size > npdm_size) {
                npdm_size = raw_hdr.acid_offset + raw_hdr.acid_size;
            }
            fseeko64(tool_ctx.file, 0, SEEK_SET);
            npdm_t *npdm = malloc(npdm_size);
            if (npdm == NULL) {
                fprintf(stderr, "Failed to allocate NPDM!\n");
                exit(EXIT_FAILURE);
            }
            if (fread(npdm, 1, npdm_size, tool_ctx.file) != npdm_size) {
                fprintf(stderr, "Failed to read NPDM!\n");
                exit(EXIT_FAILURE);
            }
            npdm_print(npdm, &tool_ctx);
            break;
        }
        case FILETYPE_HFS0: {
            hfs0_ctx_t hfs0_ctx;
            memset(&hfs0_ctx, 0, sizeof(hfs0_ctx));
            hfs0_ctx.file = tool_ctx.file;
            hfs0_ctx.tool_ctx = &tool_ctx;
            hfs0_process(&hfs0_ctx);
            if (hfs0_ctx.header) {
                free(hfs0_ctx.header);
            }
            break;
        }
        case FILETYPE_PACKAGE1: {
            pk11_ctx_t pk11_ctx;
            memset(&pk11_ctx, 0, sizeof(pk11_ctx));
            pk11_ctx.file = tool_ctx.file;
            pk11_ctx.tool_ctx = &tool_ctx;
            pk11_process(&pk11_ctx);
            if (pk11_ctx.pk11) {
                free(pk11_ctx.pk11);
            }
            break;
        }
        case FILETYPE_PACKAGE2: {
            pk21_ctx_t pk21_ctx;
            memset(&pk21_ctx, 0, sizeof(pk21_ctx));
            pk21_ctx.file = tool_ctx.file;
            pk21_ctx.tool_ctx = &tool_ctx;
            pk21_process(&pk21_ctx);
            if (pk21_ctx.sections) {
                free(pk21_ctx.sections);
            }
            break;
        }
        case FILETYPE_XCI: {
            xci_ctx_t xci_ctx;
            memset(&xci_ctx, 0, sizeof(xci_ctx));
            xci_ctx.file = tool_ctx.file;
            xci_ctx.tool_ctx = &tool_ctx;
            xci_process(&xci_ctx);
            break;
        }
        default: {
            fprintf(stderr, "Unknown File Type!\n\n");
            usage();
        }
    }
    
    if (tool_ctx.file != NULL) {
        fclose(tool_ctx.file);
    }
    printf("Done!\n");

    return EXIT_SUCCESS;
}