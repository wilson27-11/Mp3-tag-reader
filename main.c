#include <stdio.h>
#include <string.h>
#include<stdarg.h>

int syncsafe_to_int(unsigned char size[4])
{
    return ((size[0] & 0x7F) << 21) | ((size[1] & 0x7F) << 14) | ((size[2] & 0x7F) << 7) |
           (size[3] & 0x7F);
}

void int_to_syncsafe(int value, unsigned char size[4])
{
    size[0] = (value >> 21) & 0x7F;
    size[1] = (value >> 14) & 0x7F;
    size[2] = (value >> 7) & 0x7F;
    size[3] = value & 0x7F;
}

int main(int argc, char *argv[])
{


    if (argc > 1 && strcmp(argv[1], "-v") == 0)

    {
        FILE* fp;
        char  ch;

        fp = fopen("backup.mp3.mpeg", "rb");

        if (fp == NULL)
        {
            perror("ERROR ");
            return 1;
        }

        // for id3
        int  i;
        char id[4];

        for (i = 0; i < 3; i++)
        {
            ch = fgetc(fp);

            id[i] = ch;
        }
        id[i] = '\0';

        if (strcmp(id, "ID3"))
        {
            printf("\nInvalid file\n");

            fclose(fp);
            return 1;
        }

        // skip 7 byte
        fseek(fp, 7, SEEK_CUR);

        char          frame_header[5];
        unsigned char size_buf[4];
        int           size;

        printf("\n---------------------------------------------------------\n");
        printf("           MP3 Tag Reader and Editor for ID3\n");
        printf("---------------------------------------------------------\n");
        for (int j = 0; j < 6; j++)
        {
            // for frame header(4 byte)

            for (i = 0; i < 4; i++)
            {
                ch = fgetc(fp);

                frame_header[i] = ch;
            }
            frame_header[i] = '\0';

            printf("%s   :", frame_header);

            // frame size
            fread(size_buf, 1, 4, fp);

            // store the size as int
            size = (size_buf[0] << 24) | (size_buf[1] << 16) | (size_buf[2] << 8) | (size_buf[3]);

            // skip 3 position for flag and \0
            fseek(fp, 3, SEEK_CUR);

            // frame content
            char frame_content[size];
            for (i = 0; i < size - 1; i++)
            {
                ch = fgetc(fp);

                frame_content[i] = ch;
            }
            frame_content[i] = '\0';

            printf("  %s\n", frame_content);
        }
        printf("---------------------------------------------------------\n");

        fclose(fp);

        return 0;
    }

    else if (argc > 2 && strcmp(argv[1], "-e") == 0)
    {
        char tag_content[100], tag[50];

        if (strcmp(argv[2], "-t") == 0)
        {
            strcpy(tag, "TIT2");
        }

        else if (strcmp(argv[2], "-a") == 0)
        {
            strcpy(tag, "TPE1");
        }

        else if (strcmp(argv[2], "-A") == 0)
        {
            strcpy(tag, "TALB");
        }

        else if (strcmp(argv[2], "-y") == 0)
        {
            strcpy(tag, "TYER");
        }

        else if (strcmp(argv[2], "-m") == 0)
        {
            strcpy(tag, "TCON");
        }

        else if (strcmp(argv[2], "-c") == 0)
        {
            strcpy(tag, "COMM");
        }

        else
        {
            printf("\nInvalid input\n");
            printf("\n./a.out -h\n");

            return 1;
        }

        printf("Enter edit content : ");
        scanf(" %99[^\n]", tag_content);

        int len = strlen(tag_content);

        FILE *fp, *cp;
        int   ch;

            fp = fopen("backup.mp3.mpeg", "rb");
            cp = fopen("Duplicate.mp3.mpeg", "wb");

            if (fp == NULL)
            {
                perror("ERROR ");
                if (cp) fclose(cp);  // cp might have succeeded even though fp didn't
                return 1;
            }

            if (cp == NULL)
            {
                perror("ERROR ");
                fclose(fp);          // fp is known-valid here, so this one's safe and necessary
                return 1;
            }

            // for id3
            char id[4];
            int  j;
            for (j = 0; j < 3; j++)
            {
                ch = fgetc(fp);

                id[j] = ch;
            }
            id[j] = '\0';

            if (strcmp(id, "ID3"))
            {
                printf("\nInvalid file\n");

                fclose(fp);
                fclose(cp);
                return 1;
            }

            // back to starting position
            fseek(fp, 0, SEEK_SET);

            unsigned char header[10];

            if (fread(header, 1, 10, fp) != 10)
            {
                printf("Header read failed\n");

                fclose(fp);
                fclose(cp);
                return 1;
            }

            // write temporarily
            fwrite(header, 1, 10, cp);

            char          frame_header[5];
            unsigned char size_buf[4];
            int           size, size_diff = 0;

            for (int j = 0; j < 6; j++)
            {
                // frame header (4 bytes)
                size_t got = fread(frame_header, 1, 4, fp);
                if (got < 4)
                {
                    // not enough bytes left for a full frame header — stop
                    break;
                }
                fwrite(frame_header, 1, 4, cp);
                frame_header[4] = '\0';

                // peek at frame size (4 bytes), then rewind
                got = fread(size_buf, 1, 4, fp);
                if (got < 4)
                {
                    break;
                }
                fseek(fp, -4, SEEK_CUR);

                if (strcmp(frame_header, tag) != 0)
                {
                    // not the tag we're editing — copy this frame through unchanged
                    size = (size_buf[0] << 24) | (size_buf[1] << 16) | (size_buf[2] << 8) |
                           (size_buf[3]);

                    for (int k = 0; k < size + 6; k++)  // 6 = 4 size bytes + 2 flag bytes
                    {
                        ch = fgetc(fp);
                        if (ch == EOF) break;
                        fputc(ch, cp);
                    }
                }
                else
                {
                    // this is the tag we're editing — read old size/flags, substitute new content
                    unsigned char old_size_buf[4];
                    unsigned char flags[2];

                    if (fread(old_size_buf, 1, 4, fp) < 4) break;
                    if (fread(flags, 1, 2, fp) < 2) break;

                    int old_size = (old_size_buf[0] << 24) | (old_size_buf[1] << 16) |
                                   (old_size_buf[2] << 8) | old_size_buf[3];

                    size = len + 1;  // +1 for the text-encoding byte

                    size_diff += size - old_size;

                    // write new size (big-endian, 4 bytes)
                    unsigned char new_size_buf[4] = {(size >> 24) & 0xFF, (size >> 16) & 0xFF,
                                                     (size >> 8) & 0xFF, size & 0xFF};

                    fwrite(new_size_buf, 1, 4, cp);
                    fwrite(flags, 1, 2, cp);  // flags unchanged

                    fputc(0x00, cp);

                    // write new content
                    for (int k = 0; k < len; k++)
                    {
                        fputc(tag_content[k], cp);
                    }

                    // skip past the OLD content in the source file so fp stays aligned
                    fseek(fp, old_size, SEEK_CUR);
                }
            }

        int old_tag_size = syncsafe_to_int(&header[6]);

        int new_tag_size = old_tag_size + size_diff;

        int_to_syncsafe(new_tag_size, &header[6]);

        fseek(cp, 0, SEEK_SET);
        fwrite(header, 1, 10, cp);
        fflush(cp);

        fclose(fp);
        fclose(cp);

        if (remove("backup.mp3.mpeg") != 0)
        {
            perror("remove");
        }

        if (rename("Duplicate.mp3.mpeg", "backup.mp3.mpeg") != 0)
        {
            perror("rename");
            return 1;
        }

        printf("\nEdited successfully\n");

        return 0;
    }

    else if (argc > 1 && strcmp(argv[1], "-h") == 0)
    {
        printf("\n--------------------------------------------\n");
        printf("              MP3 Tag - Help\n");
        printf("--------------------------------------------\n");
        printf("1.  -v  ->  To view mp3 file content\n");
        printf("2.  -e  ->  To edit mp3 file content\n");
        printf("       2.1   -t  ->  To edit song title\n");
        printf("       2.2   -a  ->  To edit artish name\n");
        printf("       2.3   -A  ->  To edit album name\n");
        printf("       2.4   -y  ->  To edit year\n");
        printf("       2.5   -m  ->  To edit content\n");
        printf("       2.6   -c  ->  To edit comment\n");
        printf("--------------------------------------------\n");
    }

    else
    {
        printf("\nInvalid input\n");
        printf("\nTo get help menu\n./a.out -h\n");

        return 1;
    }

    return 0;
}
