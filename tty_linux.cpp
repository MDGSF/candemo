#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

void printbuf(int n, char *buf) {
  printf("n = %d, ", n);
  for (int i = 0; i < n; i++) {
    printf("%02X ", buf[i]);
  }
  printf("\n");
}

void printCamFrame(struct can_frame * p) {
  printf("%08X ", p->can_id);
  printf("[%d] ", p->can_dlc);
  for (int i = 0; i < p->can_dlc; i++) {
    printf("%02X ", p->data[i]);
  }
  printf("\n");
}

int main() {
  printf("hello world\n");

  int ret = 0;

  int fd = open("/dev/ttyUSB0", O_RDWR);
  if (fd < 0) {
    printf("open %s failed, err = %s\n", "/dev/ttyUSB0", strerror(errno));
    return -1;
  }

  struct termios tty;
  memset(&tty, 0, sizeof(tty));
  cfmakeraw(&tty);

  ret = tcgetattr(fd, &tty);
  if (ret < 0) {
    printf("tcgetattr failed, err = %s\n", strerror(errno));
    return -1;
  }

  printf("i speed = %d\n", cfgetispeed(&tty));
  printf("o speed = %d\n", cfgetospeed(&tty));

  // cfsetispeed(&tty, B600);
  // cfsetospeed(&tty, B600);

  // printf("i speed = %d\n", cfgetispeed(&tty));
  // printf("o speed = %d\n", cfgetospeed(&tty));

  tcflush(fd, TCIFLUSH);

  tcsetattr(fd, TCSANOW, &tty);
  if (ret < 0) {
    printf("tcsetattr failed, err = %s\n", strerror(errno));
    return -1;
  }

  while (1) {
    struct can_frame frame;
    char buf[256] = {0};
    int n = read(fd, &frame, sizeof(frame));
    if (n < 0) {
      printf("read failed, err = %s\n", strerror(errno));
      break;
    } else if (n == 0) {
      continue;
    }
    // printf("n = %d, buf = %s\n", n, buf);
    // printbuf(n, buf);
    printCamFrame(&frame);
  }

  return 0;
}

