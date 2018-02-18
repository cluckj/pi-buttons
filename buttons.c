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
  int l, gpioCount;
  buttonDefinition * buttons[MAX_BUTTONS];
  pthread_t buttonThread;
  pthread_t socketThread;
  int clients[MAX_CLIENTS];


  // TODO these need to come from a config file or command line args
  const char * gpios[] = {
    "17",
    "27"
  };
  gpioCount = 2;

  // init client file descriptors
  for(l = 0; l < MAX_CLIENTS; l++) {
    clients[l] = -1;
  }

  for(l = 0; l < gpioCount; l++) {
    buttons[l] = (buttonDefinition *)malloc(sizeof(buttonDefinition));
    buttons[l]->gpio = gpios[l];
    pthread_mutex_init(&buttons[l]->lockControl, NULL);
    pthread_barrier_init(&buttons[l]->barrierControl, NULL, 2);
    buttons[l]->clients = clients;
    pthread_create(&buttons[l]->parent, NULL, &buttonParent, buttons[l]);
  }

  for(l = 0; l < gpioCount; l++) {
    pthread_join(buttons[l]->parent, NULL);
  }


/*
  pthread_create(&socketThread, NULL, &socketServer, &clients);
*/

}


void * buttonParent(void * args) {
  buttonDefinition * button;
  button = (buttonDefinition *)args;
  char buff[30];
  struct pollfd pollfdStruct;
  int pollStatus;
  uint8_t c;

  sprintf(buff, "/sys/class/gpio/gpio%s/value", button->gpio);
  if ((button->fd = open(buff, O_RDWR)) < 0) {
    printf("Failed to open gpio%d.\n", button->gpio);
    exit(1);
  }

  // configure polling structure
  pollfdStruct.fd = button->fd;
  pollfdStruct.events = POLLPRI | POLLERR;

  // configure button structure
  button->state = STATE_INIT;
  button->debounceState = INACTIVE;

  // clear out any waiting gpio values
  pollStatus = poll(&pollfdStruct, 1, 10); // 10 millisecond wait for input
  if (pollStatus > 0) {
    if (pollfdStruct.revents & POLLPRI) {
      lseek (pollfdStruct.fd, 0, SEEK_SET) ;	// Rewind
      (void)read (pollfdStruct.fd, &c, 1) ;	// Read & clear
    }
  }

  // reset button state
  button->state = STATE_IDLE;
  pthread_mutex_lock(&button->lockControl);
  pthread_create(&button->child, NULL, &buttonChild, button);

  for(;;) {
    pollStatus = poll(&pollfdStruct, 1, -1) ;
    if (pollStatus > 0) {
        if (pollfdStruct.revents & POLLPRI) {
          lseek (pollfdStruct.fd, 0, SEEK_SET) ;	// Rewind
          (void)read (pollfdStruct.fd, &c, 1) ;	// Read & clear

          button->lastValue = c;
          // TODO consider conditional based on debounce state
          pthread_mutex_unlock(&button->lockControl); // signal child button event has started
          pthread_barrier_wait(&button->barrierControl); // wait on begin sychronization
          pthread_mutex_lock(&button->lockControl); // regain lock for next event
          pthread_barrier_wait(&button->barrierControl); // signal child synchronized
        }

    }
    else {
      // something wrong with poll status

    }
  }
}


void * buttonChild(void * args) {
  buttonDefinition * button;
  button = (buttonDefinition *)args;
  int lockStatus, count = 0;

  for(;;) {
    if (button->debounceState == ACTIVE) {
      lockStatus = pthread_mutex_timedlock(&button->lockControl, &button->conditionTime);
    }
    else {
      lockStatus = pthread_mutex_lock(&button->lockControl); // wait for parent to signal button event started
    }
    // TODO may need other error checks

    if (!lockStatus) {
      // we have lock, perform handshake with parent
      pthread_mutex_unlock(&button->lockControl); // release for synchronization
      pthread_barrier_wait(&button->barrierControl); // begin synchronization
      pthread_barrier_wait(&button->barrierControl); // wait for synchronized
    }

    clock_gettime(CLOCK_REALTIME, &button->lastTime);
    if (button->debounceState == INACTIVE) {
      // gpio changed, start debounce
      button->debounceState = ACTIVE;
      button->conditionTime.tv_sec = button->lastTime.tv_sec;
      button->conditionTime.tv_nsec = button->lastTime.tv_nsec + DEBOUNCE_NS;
    }
    else if (
      (
        !lockStatus &&
        ((button->lastTime.tv_sec - button->conditionTime.tv_sec) * 1000000000 +
        (button->lastTime.tv_nsec - button->conditionTime.tv_nsec) > DEBOUNCE_NS)
      ) || lockStatus == ETIMEDOUT) {
      // debounce timed out or locked after debounce
      // TODO is it even possible to lock after debounce?
      if (button->lastValue == RELEASED) {
        count++;
        printf("%s %d\n", button->gpio, count);
      }
      button->debounceState = INACTIVE;
    }
  }
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

/*
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


  for(;;) {
    pollStatus = poll(polls, ptArgs->gpioCount, -1) ;
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
*/
/*
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
*/

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
