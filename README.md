# Semáforos em C (POSIX) — SO / IFRN, Grupo 8

Simulador independente que usa semáforos reais do Linux.

| Conceito | Função |
|---|---|
| Thread | `pthread_create` / `pthread_join` |
| DOWN (P) | `sem_wait()` |
| UP (V) | `sem_post()` |
| DOWN com limite | `sem_timedwait()` (evita travar a demo de deadlock) |

## Menu
1. Produtor-consumidor com buffer de tamanho 2 (ordem correta).
2. Deadlock: produtor segura o mutex esperando `vazio`; consumidor espera o mutex.

## Executar
```
docker build -t semaforos-c .
docker run --rm -it semaforos-c
```
Ou sem Docker: `gcc -pthread -o semaforos main.c && ./semaforos`
