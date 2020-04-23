#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <thread>

using namespace std;

void printbuf(int n, char *buf) {
  printf("n = %d, ", n);
  for (int i = 0; i < n; i++) {
    printf("%02X ", buf[i]);
  }
  printf(", ");
  for (int i = 0; i < n; i++) {
    printf("%c", buf[i]);
  }
  printf("\n");
}

void printCamFrame(struct can_frame *p) {
  printf("%08X ", p->can_id);
  printf("[%d] ", p->can_dlc);
  for (int i = 0; i < p->can_dlc; i++) {
    printf("%02X ", p->data[i]);
  }
  printf(", ");
  for (int i = 0; i < p->can_dlc; i++) {
    printf("%c", p->data[i]);
  }
  printf("\n");
}

void ThreadProxyStdin2Can(int fd) {
  char buf[8] = {0};
  struct can_frame frame;
  frame.can_id = 0x123;
  while (1) {
    ssize_t nr = read(STDIN_FILENO, buf, sizeof(buf));
    if (nr < 0) {
      printf("read stdin failed, err = %s\n", strerror(errno));
      break;
    } else if (nr == 0) {
      continue;
    }
    frame.can_dlc = nr;
    memcpy(frame.data, buf, nr);

    // printbuf(nr, buf);

    ssize_t nw = write(fd, &frame, sizeof(frame));
    if (nw <= 0) {
      printf("write to can failed, err = %s\n", strerror(errno));
      break;
    }
    if (nw != sizeof(frame)) {
      printf("write to can failed, err = %s, nr = %ld, nw = %ld\n",
             strerror(errno), nr, nw);
      break;
    }
  }
}

void ThreadProxyCan2Stdout(int fd) {
  while (1) {
    // TODO epoll
    struct can_frame frame;
    int n = read(fd, &frame, sizeof(frame));
    if (n < 0) {
      printf("read failed, err = %s\n", strerror(errno));
      break;
    } else if (n == 0) {
      continue;
    }
    printCamFrame(&frame);
  }
}

int createCanFd() {
  int ret = 0;

  int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (fd < 0) {
    printf("create can socket fd failed, err = %s\n", strerror(errno));
    return -1;
  }

  struct ifreq ifr;
  strcpy(ifr.ifr_name, "can0");
  ret = ioctl(fd, SIOCGIFINDEX, &ifr);
  if (ret < 0) {
    printf("ioctl SIOCGIFINDEX failed, err = %s\n", strerror(errno));
    return -1;
  }

  // bind fd to can0
  struct sockaddr_can addr;
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  bind(fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    printf("bind fd to can failed, err = %s\n", strerror(errno));
    return -1;
  }

  //设置规则，只接受 can_id = 0x123 的数据包
  struct can_filter rfilter[1];
  rfilter[0].can_id = 0x123;
  rfilter[0].can_mask = CAN_SFF_MASK;
  setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
  if (ret < 0) {
    printf("setsockopt CAN_RAW_FILTER failed, err = %s\n", strerror(errno));
    return -1;
  }

  // 开启之后，可以收到由自己这个 fd 发送出去的消息。
  // int recvOwnMsg = 1;
  // setsockopt(fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recvOwnMsg,
  //            sizeof(recvOwnMsg));
  // if (ret < 0) {
  //   printf("setsockopt CAN_RAW_RECV_OWN_MSGS failed, err = %s\n", strerror(errno));
  //   return -1;
  // }

  // 回环功能，0 表示关闭, 1 表示开启( 默认)
  // int loopback = 0;
  // setsockopt(fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));

  return fd;
}

int main() {
  // TODO 命令行参数 can0 frame_id
  setbuf(stdout, NULL);

  int fd = createCanFd();
  if (fd < 0) {
    return -1;
  }

  thread th1(ThreadProxyStdin2Can, fd);
  thread th2(ThreadProxyCan2Stdout, fd);

  th2.join();
  th1.join();
  return 0;
}
