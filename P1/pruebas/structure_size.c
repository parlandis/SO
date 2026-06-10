 #include <stdio.h>
  
  int main() {
      FILE *archivo;
      unsigned char datos[5] = { 10, 20, 30, 40, 50 };
  
      archivo = fopen("datos.bin", "wb");  // Modo escritura binaria
      if (archivo == NULL) {
          fprintf(stderr, "No se pudo crear el archivo binario\n");
          return 1;
      }
  
      fwrite(datos, sizeof(char), 5, archivo);  // Escribe 5 bytes
      fclose(archivo);  // Cierra el archivo
  
      // Ahora leemos los datos del archivo binario
      archivo = fopen("datos.bin", "rb");  // Modo lectura binaria
      if (archivo == NULL) {
          fprintf(stderr, "No se pudo abrir el archivo binario\n");
          return 1;
      }
  
      unsigned char byte;
      while (fread(&byte, sizeof(char), 1, archivo) > 0) {
          printf("%d ", byte);  // Imprime cada byte
      }
      printf("\n");
  
      fclose(archivo);  // Cierra el archivo
      return 0;
  }
