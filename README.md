# Cliente-Servidor SSH

El programa permite la ejecución de código de forma remota a través de una conexión TCP.
El cliente se conecta al servidor y tras enviar comandos recibe los resultados de ejecución de dicho comando.

---

## Compilación

### Servidor
```
gcc -o servidor servidor.c
```

### Cliente
```
gcc -o cliente cliente.c
```
---

## Ejecución

### Servidor
```
./servidor {Puerto}
```
> Remplace *{Puerto}* por el puerto requerido

### Cliente
```
./cliente {DireccionIP} {Puerto}
```
> Remplace *{DireccionIP}* *{Puerto}* por la dirección IP y el puerto del servidor

