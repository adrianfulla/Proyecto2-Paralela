# Proyecto 2 - Open MPI
## Universidad del Valle de Guatemala
### CC3069 - Computación paralela y distribuida
### Autores
- Emilio José Solano Orozco, 21212
- Adrian Fulladolsa Palma, 21592
- Elías Alberto Alvarado Raxón, 21808


## Descripción

En equipos de 3 integrantes, deben utilizar MPI para diseñar un programa que encuentre la
llave privada con la que fue cifrado un texto plano. La búsqueda se hará probando todas las posibles
combinaciones de llaves, hasta encontrar una que descifra el texto (fuerza bruta). Se sabrá si logro
descifrar el texto correctamente validando si el mismo contiene como substring una palabra/frase clave
de búsqueda, la cual se sabe a priori (por ejemplo, si el texto cifrado fuera el presente párrafo,
‘combinaciones’ podría ser una buena palabra clave).
Es importante elegir una frase clave a buscar de buen tamaño, para asegurar que sea muy improbable
que esa clave suceda de forma aleatoria al descifrar (erróneamente) el texto.
Además del cifrado, descifrado y paralelización con Open MPI, el equipo debe realizar un análisis de
speedups y performance para entender y optimizar la distribución del espacio de búsqueda de las llaves.
Deberá ponerse especial atención al impacto que tiene la llave solución en el performance y
tiempos/speedups aparentes del algoritmo, así como la consistencia de este con distintas llaves.


## Prerequisitos
- Sistema Operativo Linux -> https://www.linux.org/pages/download/
- Compilador de C++/g++ -> Generalmente incluido en distros de Linux
- Git -> https://git-scm.com/downloads
- OpenMPI -> https://www.open-mpi.org/software/ompi/v5.0/
- OpenSSL -> https://github.com/openssl/openssl

## Instalación

1. Clonar repositorio 
```bash
git clone https://github.com/adrianfulla/Proyecto2-Paralela.git
```

2. Entrar al directorio del repositorio
```bash
cd Proyecto2-Paralela/
```

### Parte A

3. Compilar programa secuencial
```bash
gcc parte1Seq.c -o parte1Seq -lssl -lcrypto
```

4. Compilar programa bruteforce
```bash
mpicc -o brutefoce bruteforce.c -lssl -lcrypto
```

### Parte B

5. Compilar programa secuencial con lectura de archivo .txt
```bash
gcc parteBSeq.c -o parteBSeq -lssl -lcrypto
```

6. Compilar programa paralelo con lectura de archivo .txt
```bash
mpicc -o brutefoce_partB bruteforce_partB.c -lssl -lcrypto
```

7. Compilar solucion 1
```bash
mpicc -o solucion1 solucion1.c -lssl -lcrypto
```

8. Compilar solucion 2
```bash
mpicc -o solucion2 solucion2.c -lssl -lcrypto
```

## Ejecución de 

### Parte A
Programa secuencial
 ```bash
./parte1Seq
```


Programa paralelo
 ```bash
mpirun -np bruteforce
```

### Parte B
Programa secuencial con lectura de archivo .txt
 ```bash
./parteBSeq <Nombre del archivo .txt> <Palabra Clave> <Llave privada>
```


Programa bruteforce con lectura de archivo .txt
 ```bash
mpirun -np bruteforce_partB <Nombre del archivo .txt> <Palabra Clave> <Llave privada>
```

Programa solucion1 con lectura de archivo .txt
 ```bash
mpirun -np solucion1 <Nombre del archivo .txt> <Palabra Clave> <Llave privada>
```

Programa solucion2 con lectura de archivo .txt
 ```bash
mpirun -np solucion2 <Nombre del archivo .txt> <Palabra Clave> <Llave privada>
```





## Notas

- Dentro de este repositorio se provee el archivo example.txt que contiene el siguiente texto:
`Esta es una prueba del Proyecto 2`

- Al utilizar la solucion 2, se sugiere el uso de una palabra clave que se encuentre al inicio del texto que sera cifrado. En caso se utilice una palabra clave que no este al inicio no es posible validar el cifrado.
