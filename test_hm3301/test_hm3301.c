#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>

#define DEVFILE "/dev/leds"
#define LED_NUM 3
#define LED_ON 1
#define LED_OFF 0

#define HWMON_HM3301_PM25 "/sys/class/hwmon/hwmon2/temp1_input"
#define HIGH_PM25_CONCENTRATION 40
#define MIDDLE_PM25_CONCENTRATION 20
#define LOW_PM25_CONCENTRATION 0

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int init()
{
    int fd = open(DEVFILE, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        return -1;
    }

    for (int led = 0; led < LED_NUM; led++)
    {
        ioctl(fd, LED_OFF, led);
    }

    close(fd);
    return 0;
}

int main(int argc, char **argv)
{
    int fd;
    int led = 0, value = 0;
    int prev_led = -1; // Initialize previous LED to an invalid value

    // Initialize LEDs
    if (init() == -1)
    {
        fprintf(stderr, "LED initialization failed\n");
        return 1;
    }

    while (1)
    {
        fd = open(HWMON_HM3301_PM25, O_RDONLY);
        if (fd == -1)
        {
            perror("open");
            break;
        }

        // Read PM2.5 concentration
        char pm25_str[10]; // Assuming a maximum of 10 characters for the PM2.5 value
        if (read(fd, pm25_str, sizeof(pm25_str)) == -1)
        {
            perror("read");
            close(fd);
            break;
        }
        close(fd);

        // Convert string to integer
        unsigned short pm25 = (unsigned short)atoi(pm25_str);

        fd = open(DEVFILE, O_RDWR);

        if (fd == -1)
        {
            perror("open");
            break;
        }
        // printf("pm2.5|%u\n", pm25);
        // Toggle LED based on PM2.5 concentration
        if (pm25 >= LOW_PM25_CONCENTRATION && pm25 < MIDDLE_PM25_CONCENTRATION)
        {
            led = 0;
            value = LED_ON;
        }
        else if (pm25 >= MIDDLE_PM25_CONCENTRATION && pm25 < HIGH_PM25_CONCENTRATION)
        {
            led = 1;
            value = LED_ON;
        }
        else if (pm25 >= HIGH_PM25_CONCENTRATION)
        {
            led = 2;
            value = LED_ON;
        }
        else // If PM2.5 concentration is below LOW_PM25_CONCENTRATION
        {
            led = -1; // Invalid LED
            value = LED_OFF;
        }

        pthread_mutex_lock(&mutex);
        if (led != prev_led)
        {
            if (prev_led != -1)
            {
                ioctl(fd, LED_OFF, prev_led); // Turn off previous LED
            }
            if (led != -1)
            {
                ioctl(fd, value, led); // Turn on current LED
            }
            prev_led = led; // Update previous LED
        }
        pthread_mutex_unlock(&mutex);

        close(fd);

        sleep(1);
    }

    return 0;
}
