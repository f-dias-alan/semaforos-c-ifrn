FROM gcc:13
WORKDIR /app
COPY main.c .
RUN gcc -Wall -Wextra -pthread -o semaforos main.c
CMD ["./semaforos"]
