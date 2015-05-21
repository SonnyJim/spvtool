#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

/* File format:
 * Header size: 28 bytes
 * dword 0      = type header (SPV1)
 * dword 1      = ?
 * dword 2      = Animation frame delay/advance? 0 for no advance
 * dword 3      = xres
 * dword 4      = yres
 * dword 5      = Bytes per pixel maybe?
 * dword 6      = number of frames
 * Frame data = 153600 bytes (320x240x2) + 8 bytes of ?
 *
 * Extra bytes observations:
 * 
 * 4th byte of extra data on the end of the first frame always matches the 4th byte of the first frame
 * Is also true for frames 1, 133, 280, 494, 530, 613, 696, 722
 */

#define HDR_SIZE (sizeof(int32_t) * 7)
#define FRAME_SIZE ((spvfile.xres * spvfile.yres * spvfile.bpp))
//Length of mystery bits
#define BIT_LEN 8
#define DWORD sizeof(int32_t)

struct spvfile
{
    FILE *fp;
    char type[12];
    char *shortname;
    char *filename;
    int32_t xres;
    int32_t yres;
    int32_t bpp;
    long frames;
    long filesize;
} spvfile = {NULL, "", "", NULL, 0, 0, 0, 0, 0};

void print_usage (void)
{
	fprintf (stdout, "Usage:\n");
	fprintf (stdout, "spvtool [option] filename\n");
    fprintf (stdout, "-d    Dump individual frames\n");
}

void print_spvinfo (void)
{
    fprintf (stdout, "Filename: %s\n", spvfile.filename);
    fprintf (stdout, "Shortname: %s\n", spvfile.shortname);
    fprintf (stdout, "Filesize: %li\n", spvfile.filesize);
    fprintf (stdout, "type: %s\n", spvfile.type);
    fprintf (stdout, "xres: %i\n", spvfile.xres);
    fprintf (stdout, "yres: %i\n", spvfile.yres);
    fprintf (stdout, "bytes per pixel: %i\n", spvfile.bpp);
    fprintf (stdout, "frames: %li\n", spvfile.frames);
}

//Strip directories and extension off filename
char * strip_filename (void)
{
    char *start, *end;
    char *stripped;
    
    stripped = malloc (strlen (spvfile.filename));
    if (stripped == NULL)
    {
        fprintf (stdout, "Error mallocing stripped filename\n");
        return NULL;
    }

    start = strrchr (spvfile.filename, '/');
    if (start == NULL)
        start = spvfile.filename;
    else
        start++;
    
    strcpy (stripped, start);

    end = strrchr (stripped, '.');
    if (start != NULL)
        *end = '\0';

    return stripped;
}

int create_outputdir (void)
{
    int ret;

    ret = mkdir (spvfile.shortname, S_IRWXU);

    if (ret)
    {
        if (errno == EEXIST)
            return 0;
    }
    return ret;
}

int read_spvinfo (void)
{
   
    fseek (spvfile.fp, 0, SEEK_SET);
    //Read type
    fread (spvfile.type, DWORD * 3, 1, spvfile.fp);
   
    if (strncmp (spvfile.type, "SPV1", 4) != 0)
        return 1;
    //Read dimensions
    fread (&spvfile.xres, DWORD, 1, spvfile.fp);
    fread (&spvfile.yres, DWORD, 1, spvfile.fp);
 
    //Read bytes per pixel?  Pure guess at this point
    fread (&spvfile.bpp, DWORD, 1, spvfile.fp);

    //Read number of frames
    fread (&spvfile.frames, DWORD, 1, spvfile.fp);

    //Get filesize
    fseek (spvfile.fp, 0, SEEK_END);
    spvfile.filesize = ftell (spvfile.fp);
    fseek (spvfile.fp, 0, SEEK_SET);
    
    spvfile.shortname = strip_filename ();

    return 0;
}

/*
void read_frame (void)
{
    FILE *output;
    int32_t *frame;
    frame = malloc (FRAME_SIZE);
    if (frame == NULL)
    {
        fprintf (stdout, "Failed to malloc %i bytes for frame\n", FRAME_SIZE);
        return;
    }
    output = fopen ("frame.data", "w");
    
    if (output == NULL)
    {
        fprintf (stdout, "Error opening output file for writing\n");
        return;
    }
    fseek (spvfile.fp, HDR_SIZE, SEEK_SET);
    
    fread (frame, 1, FRAME_SIZE, spvfile.fp);
    fwrite (frame, 1, FRAME_SIZE, output);
    free (frame);
    fclose (output);
}
*/


//function to dump the 'mystery bits'
void dump_bits (void)
{
    FILE *bits;
    int *buffer;
    int frame_count = 0;
    char *bitsname;

    bitsname = malloc (strlen (spvfile.shortname) + strlen ("/bits.bin") + 1);
    strcpy (bitsname, spvfile.shortname);
    strcat (bitsname, "/bits.bin\0");

    buffer = malloc (BIT_LEN);
    bits = fopen (bitsname, "w");

    fseek (spvfile.fp, HDR_SIZE + FRAME_SIZE, SEEK_SET);

    fprintf (stdout, "Dumping the 8 mystery bytes\n");
    while (frame_count < spvfile.frames)
    {
        frame_count++;
        fread (buffer, 1, BIT_LEN, spvfile.fp);
        fwrite (buffer, 1, BIT_LEN, bits);

        fseek (spvfile.fp, FRAME_SIZE, SEEK_CUR);
    }

    free (buffer);
    fclose (bits);
}

void dump_frames (void)
{
    FILE *output_frame;
    char *output_filename;
    int *buffer;
    int frame_count = 0;
   
    output_filename = malloc (strlen (spvfile.shortname) + strlen ("/frame0000.data") + 1);

    buffer = malloc (FRAME_SIZE);
    fseek (spvfile.fp, HDR_SIZE, SEEK_SET);

    fprintf (stdout, "Dumping raw frame data\n");
    while (frame_count < spvfile.frames)
    {
        frame_count++;
        sprintf (output_filename, "%s/frame%04i.data", spvfile.shortname, frame_count);
        output_frame = fopen (output_filename, "w");
        fread (buffer, 1, FRAME_SIZE, spvfile.fp);
        fwrite (buffer, 1, FRAME_SIZE, output_frame);
        fseek (spvfile.fp, BIT_LEN, SEEK_CUR);
        fclose (output_frame);
    }
    free (buffer);
}

void check_byte (void)
{
    /* Check to see if 4th byte of first frame matches
     * 4th byte of 'extra bytes'
     */

    char byte1;
    char byte2;
    int frame_count;
    int location;

    fprintf (stdout, "Checking for 4th byte matches:\n");

    fprintf (stdout, "------------------------\n");
    fprintf (stdout, "Frame | B1 | B2 | Offset\n");
    fprintf (stdout, "------------------------\n");
    for (frame_count = 0; frame_count < spvfile.frames; frame_count++)
    {
        //Seek to the 4th byte of the frame
        fseek (spvfile.fp, HDR_SIZE + ((FRAME_SIZE + BIT_LEN) * frame_count) + 3, SEEK_SET);
        fread (&byte1, 1, 1, spvfile.fp);
        location = ftell (spvfile.fp);
        //Seek to the 4th byte of the 'extra bytes', the previous call to fread moved the fp one byte
        fseek (spvfile.fp, FRAME_SIZE - 1, SEEK_CUR);
        fread (&byte2, 1, 1, spvfile.fp);

        if (byte1 == byte2)
        {
            fprintf (stdout, "%05i | %02X | %02X | %06X\n", frame_count + 1, byte1, byte2,location - 1);
        }
    }
}

int main (int argc, char **argv)
{
    int output_frames = 0;
    int c, index;

    while ((c = getopt (argc, argv, "d")) != -1)
    {
        switch (c)
        {
            case 'd':
                output_frames = 1;
                break;
            default:
                print_usage ();
        }
    }

    //Open file for reading
    for (index = optind; index < argc; index++)
    {
        spvfile.filename = argv[index];
        break;
    }

    spvfile.fp = fopen (spvfile.filename, "r");

    if (spvfile.fp != NULL)
        fprintf (stdout, "Opening SPV file\n");
    else
    {
        fprintf (stdout, "Error opening %s for reading\n", spvfile.filename);
        return 1;
    }
    
    //Read info from file
    if (read_spvinfo ())
    {
        fprintf (stdout, "Error reading header.  Are you sure it's a SPIKE video file?\n");
        return 1;
    }
    print_spvinfo ();

    if (create_outputdir ())
    {
        fprintf (stderr, "Error creating output directory %s\n", spvfile.shortname);
        return 1;
    }
    else
        fprintf (stdout, "Writing output to %s/\n", spvfile.shortname);

    dump_bits ();
    if (output_frames)
        dump_frames ();
    check_byte ();
    fclose (spvfile.fp);
    return 0;
}
