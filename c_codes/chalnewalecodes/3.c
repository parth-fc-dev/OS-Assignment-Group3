#include <stdio.h>

#include <stdlib.h>

#include <alsa/asoundlib.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <termios.h>

#include <strings.h>

#include <unistd.h>

#define BAUDRATE B9600

#define MODEMDEVICE "/dev/ttyUSB0"

#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0

#define TRUE 1

volatile int STOP = FALSE;

int count = 50;

// Function to set the volume

int set_volume(long volume)
{

    int err;

    long min, max;

    snd_mixer_t *handle;

    snd_mixer_selem_id_t *sid;

    const char *card = "default";

    const char *selem_name = "Master";

    // Open mixer

    if ((err = snd_mixer_open(&handle, 0)) < 0)
    {

        printf("Can't open mixer %s\n", snd_strerror(err));

        return -1;
    }

    // Attach mixer

    if ((err = snd_mixer_attach(handle, card)) < 0)
    {

        printf("Can't attach mixer %s\n", snd_strerror(err));

        snd_mixer_close(handle);

        return -1;
    }

    // Register simple mixer element

    if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0)
    {

        printf("Can't register mixer element %s\n", snd_strerror(err));

        snd_mixer_close(handle);

        return -1;
    }

    // Load mixer elements

    if ((err = snd_mixer_load(handle)) < 0)
    {

        printf("Can't load mixer elements %s\n", snd_strerror(err));

        snd_mixer_close(handle);

        return -1;
    }

    snd_mixer_selem_id_alloca(&sid);

    snd_mixer_selem_id_set_index(sid, 0);

    snd_mixer_selem_id_set_name(sid, selem_name);

    snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);

    // Get volume range

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    // Set volume

    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    // Close mixer

    snd_mixer_close(handle);

    return 0;
}

int main(int argc, char *argv[])
{

    int fd, c, res;

    struct termios oldtio, newtio;

    char buf[255];

    long volume = 50; // Set the desired volume (0 to 100)

    fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(MODEMDEVICE);
        exit(-1);
    }

    tcgetattr(fd, &oldtio); /* save current port settings */

    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;

    newtio.c_iflag = IGNPAR;

    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */

    newtio.c_cc[VMIN] = 3; /* blocking read until 2 chars received */

    tcflush(fd, TCIFLUSH);

    tcsetattr(fd, TCSANOW, &newtio);

    buf[res] = 0;

    while (STOP == FALSE)
    { /* loop for input */

        res = read(fd, buf, 255); /* returns after 5 chars have been input */

        /* so we can printf... */

        printf("%s\n", buf);

        if (buf[0] == 'z')
            STOP = TRUE;

        count--;

        volume = atoi(buf);

        if (set_volume(volume) == -1)
        {

            printf("Failed to set volume.\n");

            return -1;
        }
        else
        {

            printf("Volume set to %ld%%\n", volume);
        }

        tcsetattr(fd, TCSANOW, &oldtio);
    }

    return 0;
}