#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

	// Chequeo de parámetros. Se recibe N que es el número hasta el cual se quiere calcular la suma de los números naturales.
	if (argc != 2)
	{
		printf("Uso: %s N\n", argv[0]);
		return 1;
	}

	unsigned long N = atol(argv[1]);
	unsigned long sum = 0;
	int i;

	// Calcula la sumatoria de los N numeros consecutivos
	for (i = 1; i <= N; i++)
	{
		sum += i;
	}

	// Compara la suma anterior con la suma de Gauss, deben ser iguales
	if (sum == ((N * (N + 1)) / 2))
	{
		printf("Resultado Correcto\n");
	}
	else
	{
		printf("ERROR\n");
	}
}

/*
	Es incorrecta porque el resultado de la suma de los N números naturales puede
	exceder el rango de un entero sin signo (unsigned long) para valores grandes de N,
	lo que provoca un desbordamiento y un resultado incorrecto.
	Para evitar esto, se debería utilizar un tipo de dato más grande, como unsigned
	long long, o implementar una función que calcule la suma sin riesgo de desbordamiento.
*/