# Sistemas Operativos: Proyecto 1 Chat
Proyecto 1 Chat en linea para el curso Sistemas Operativos


## Configuración del Ambiente
Este fragmento explica como configurar el ambiente de docker usando el `Dockerfile`
del repositorio. 

### Pre-requisitos
* Docker instalado en tu máquina ([Docker installation guide](https://docs.docker.com/get-docker/))

### Construyendo la Imagen de docker
1. Después de haber clonado el repositorio a tu máquina, dirígete al directorio que contiene
el repositorio a través de tu terminal. 
2. Posteriormente corre el siguiente comando:
```bash
docker build -t proyecto-sistos .
```
3. Una vez creada la imagen, tenemos que crear el contenedor, para esto corremos el siguiente comando
```bash
docker run -d -p 22000:22000 -v .:/home/project --name proyecto-container proyecto-sistos
```

4. Ahora finalmente podemos accesar a nuestro contenedor via el siguiente comando:
```bash
docker exec -it proyecto-container /bin/bash
```


## Compilación del código
Para compilar nuestro códogo primero debemos asegurarnos de tener la version transpilada de los 
archivos `.proto` a una version en C, para eso debemos usar el siguiente comando:
```bash
protoc --c_out=/home/project/ --proto_path=/home/project/ chat_protocol.proto
```
Una vez tengamos los archivos transpilados, compilamos el código de la siguiente forma
```bash
gcc <c_file_name> -o <output_name> proto/chat_protocol.pb-c.c -lprotobuf-c
```
