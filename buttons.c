// configure INPUT

// poll INPUT

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <string.h>

#include "buttons.h"


int main(int argc, char *argv[]) {
  char *gpios[MAX_BUTTONS];
  int l, gpioCount;
  pollerThreadArgs ptArgs;
  pthread_t buttonThread;
  pthread_t socketThread;
  int clients[MAX_CLIENTS];

  gpios[0] = "17"; // TODO these need to come from command line args
  gpios[1] = "27";
  gpioCount = 2;
  ptArgs.gpios = gpios;
  ptArgs.gpioCount = gpioCount;
  ptArgs.clients = clients;

  for(l = 0; l < MAX_CLIENTS; l++) {
    clients[l] = -1;
  }

  pthread_create(&socketThread, NULL, &socketServer, &clients);
  pthread_create(&buttonThread, NULL, &buttonPoller, &ptArgs);
  pthread_join(buttonThread, NULL); // TODO what about socket thread?

//  for(;;) { sleep(1); }

/*

clock_gettime(CLOCK_MONOTONIC_RAW, &eventTime);

event_us = eventTime.tv_sec * 1000000 + eventTime.tv_nsec / 1000;
//    time ( &rawtime );
//    timeinfo = localtime ( &rawtime );
    if (event_us - last_us > 30000 && c != last_c) {
      printf("x: %d c: %d %" PRIu64 " %" PRIu64 "\n", pollStatus, (c - 48), (event_us - last_us), event_us);
      last_us = event_us;
      last_c = c;
    }
  }
  */
}


void * socketServer(void * args) {
  int * clients;
  clients = (int *)args;
  int l, fd, socket = openSocket();

  while (1) {
    // wait for connection
    if ( (fd = accept(socket, NULL, NULL)) == -1) {
      fprintf(stderr, "Error accepting incoming connection.\n");
      continue;
    }

    for(l = 0; l < MAX_CLIENTS; l++) {
      if (clients[l] == -1) {
        clients[l] = fd;
        break;
      }
    }
    // add connection to empty slot
printf("Connect %d\n", fd);
  }
}


void * buttonPoller(void * args) {
  pollerThreadArgs * ptArgs;
  ptArgs = (pollerThreadArgs *)args;
  int l, fd, pollStatus;
  uint8_t c;
  char buff[30];
  struct pollfd polls[MAX_BUTTONS];
  gpioButton buttons[MAX_BUTTONS];
  pthread_t debounceThread;

  // open file descriptors for gpio inputs
  for(l = 0; l < ptArgs->gpioCount; l++) {
    // TODO validate length of gpio string
    sprintf(buff, "/sys/class/gpio/gpio%s/value", ptArgs->gpios[l]);
    if ((fd = open(buff, O_RDWR)) < 0) {
      printf("Failed to open gpio%s.\n", ptArgs->gpios[l]);
      exit(1);
    }

    // configure polling structure
    polls[l].fd = fd;
    polls[l].events = POLLPRI | POLLERR;

    // configure button structure
    buttons[l].index = l;
    buttons[l].fd = fd;
    buttons[l].state = STATE_INIT;
    buttons[l].debouncing = 0;
    buttons[l].clients = ptArgs->clients;
  }

  // clear out any waiting gpio values
  pollStatus = poll(polls, ptArgs->gpioCount, 10); // 10 millisecond wait for input
  if (pollStatus > 0) {
    for(l = 0; l < ptArgs->gpioCount; l++) {
      if (polls[l].revents & POLLPRI) {
        lseek (polls[l].fd, 0, SEEK_SET) ;	// Rewind
        (void)read (polls[l].fd, &c, 1) ;	// Read & clear
      }
    }
  }

  // reset button state
  for(l = 0; l < ptArgs->gpioCount; l++) {
    buttons[l].state = STATE_IDLE;
  }


/*

  // TODO need a way to read in command line args for settings


  // TODO use define for button value, "0" == 48 == PRESSED, "1" == 49 == RELEASED

*/
  for(;;) {
    // TODO count needs to be used in place of static 2
    pollStatus = poll(polls, 2, -1) ;
    if (pollStatus > 0) {
      for(l = 0; l < ptArgs->gpioCount; l++) {
        if (polls[l].revents & POLLPRI) {
          lseek (polls[l].fd, 0, SEEK_SET) ;	// Rewind
          (void)read (polls[l].fd, &c, 1) ;	// Read & clear

          if (buttons[l].debouncing) {
            // in debounce state, note last value
            buttons[l].lastValue = c;
          }
          else {
            // save value and start debounce
            buttons[l].debouncing = 1;
            buttons[l].value = c;
            buttons[l].lastValue = c;
            printf("Debounce\n");
            pthread_create(&debounceThread, NULL, &buttonDebounce, &buttons[l]);
          }
        }
      }

    }
    else {
      // something wrong with poll status

    }
  }

}


void * buttonDebounce(void * args) {
  gpioButton * button;
  button = (gpioButton *) args;
  char msg[1024];
  usleep(300000);
  button->debouncing = 0;
  sprintf(msg, "EMIT button_changed %d %c %d %c\n\n\n", button->index, button->value, button->value, button->lastValue);
  printf("%s", msg);
  emitMessage(EVENT_STRING[button_changed], button->clients);
  //writeSocket(pargs->eventFD, msg);
  // process state with emit / timeout
  // need a way to stop running emit timeout

}


void emitMessage(const char * msg, int * clients) {
  int l, wl;
  for(l = 0; l < MAX_CLIENTS; l++) {
    if (clients[l] != -1) {
      printf("WRITE: %d\n", clients[l]);
      wl = send(clients[l], msg, strlen(msg), MSG_NOSIGNAL);
      printf("WL: %d\n", wl);
      if (wl == -1) {
        // failure, remove client
        close(clients[l]);
        clients[l] = -1;
      }
    }
  }
}


int openSocket() {
  struct sockaddr_un addr;
  int fd;

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Error opening event socket.");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, EVENT_SOCKET_PATH, sizeof(addr.sun_path)-1);
  unlink(EVENT_SOCKET_PATH);

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    fprintf(stderr, "Event socket bind error.");
    exit(-1);
  }

  if (listen(fd, 5) == -1) {
    fprintf(stderr, "Event socket listen error.");
    exit(-1);
  }

  return fd;
}
