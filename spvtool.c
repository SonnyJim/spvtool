#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

/* File format:
 * dword 0 - 1  = type header (SPV1)
 * dword 2      = Animation frame delay/advance? 0 for no advance
 * dword 3      = xres
 * dword 4      = yres
 * dword 5      = Bytes per pixel maybe?
 * dword 6      = number of frames
 * Frame data = 153600 bytes + 8 bytes of ?
 */

#define HDR_SIZE (sizeof(int32_t) * 7)
#define FRAME_SIZE ((spvfile.xres * spvfile.yres * spvfile.bpp))

struct spvfile
{
    FILE *fp;
    char type[12];
    char *filename;
    int32_t xres;
    int32_t yres;
    int32_t bpp;
    long frames;
} spvfile = {NULL, "", NULL, 0, 0, 0, 0};

void print_usage (void)
{
	fprintf (stdout, "Usage:\n");
	fprintf (stdout, "spvtool filename\n");
}

void print_spvinfo (void)
{
    fprintf (stdout, "Filename: %s\n", spvfile.filename);
    fprintf (stdout, "type: %s\n", spvfile.type);
    fprintf (stdout, "xres: %i\n", spvfile.xres);
    fprintf (stdout, "yres: %i\n", spvfile.yres);
    fprintf (stdout, "bytes per pixel: %i\n", spvfile.bpp);
    fprintf (stdout, "frames: %li\n", spvfile.frames);
}

void read_spvinfo (void)
{
    int *buffer;
    buffer = malloc (sizeof(int32_t));
    
    fseek (spvfile.fp, 0, SEEK_SET);
    //Read type
    fread (spvfile.type, sizeof(buffer) * 3, 1, spvfile.fp);
    
    //Read dimensions
    fread (buffer, sizeof(buffer), 1, spvfile.fp);
    spvfile.xres = *buffer;
    fread (buffer, sizeof(buffer), 1, spvfile.fp);
    spvfile.yres = *buffer;
 
    //Read bytes per pixel?  Pure guess at this point
    fread (buffer, sizeof(buffer), 1, spvfile.fp);
    spvfile.bpp = *buffer;

    //Read number of frames
    fread (buffer, sizeof(buffer), 1, spvfile.fp);
    spvfile.frames = *buffer;
    free (buffer);
}

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
    fprintf (stdout, "Frame size %i, Header size %i\n", FRAME_SIZE, HDR_SIZE);
    fseek (spvfile.fp, HDR_SIZE, SEEK_SET);
    
    fread (frame, 1, FRAME_SIZE, spvfile.fp);
    fwrite (frame, 1, FRAME_SIZE, output);
    free (frame);
    fclose (output);
}

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        print_usage ();
        return 1;
    }
   
    //Open file for reading
    spvfile.filename = argv[1];
    spvfile.fp = fopen (spvfile.filename, "r");

    if (spvfile.fp != NULL)
        fprintf (stdout, "Opening %s\n", spvfile.filename);
    else
    {
        fprintf (stdout, "Error opening %s for reading\n", spvfile.filename);
        return 1;
    }
    
    //Read info from file
    read_spvinfo ();
    print_spvinfo ();
    read_frame ();
    fclose (spvfile.fp);
    return 0;
}
