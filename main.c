#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SEED 0x12345678

typedef struct
{
  char codigoIBGE[50];
  char nome[50];
  int codigoUf;
  int capital;
  float latitude;
  float longitude;
  int siafi;
  int ddd;
  char fusoHorario[50];
} tMunicipio;

char *get_key(char *reg) { return (*((tMunicipio *)reg)).codigoIBGE; }

void *aloca_municipio(char *codigoIBGE, char *nome, float latitude,
                      float longitude, int capital, int codigoUf, int siafi,
                      int ddd, char *fusoHorario)
{
  tMunicipio *municipio = malloc(sizeof(tMunicipio));
  strcpy(municipio->codigoIBGE, codigoIBGE);
  strcpy(municipio->nome, nome);
  municipio->codigoUf = codigoUf;
  municipio->capital = capital;
  municipio->latitude = latitude;
  municipio->longitude = longitude;
  municipio->siafi = siafi;
  municipio->ddd = ddd;
  strcpy(municipio->fusoHorario, fusoHorario);
  return municipio;
}

typedef struct
{
  uintptr_t *table;
  int size;
  int max;
  int deleted;
  char *(*get_key)(char *);
} tHash;

// Função de hash 1
uint32_t hashf1(const char *str, uint32_t h)
{
  /* One-byte-at-a-time Murmur hash
  Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
  for (; *str; ++str)
  {
    h ^= *str;
    h *= 0x12345678;
    h ^= h >> 15;
  }
  return h;
}

uint32_t hashf2(const char *str, uint32_t h)
{
  /* One-byte-at-a-time Murmur hash
  Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
  for (; *str; ++str)
  {
    h ^= *str;
    h *= 0x2bd1e995;
    h ^= h >> 15;
  }
  return h;
}

int inserir_hash(tHash *h, void *bucket)
{
  uint32_t tempHash1 = hashf1(h->get_key(bucket), SEED); // Índice inicial
  int hash1 = tempHash1 % (h->max);
  if (h->max == (h->size + 1))
  {
    free(bucket);
    return EXIT_FAILURE;
  }

  if (!h->table[hash1])
  {
    h->table[hash1] = (uintptr_t)bucket; // Espaço vazio, insere diretamente
    h->size++;
  }
  else
  {
    // Colisão, calcula o próximo índice usando a segunda função de hash
    uint32_t tempHash2 = hashf2(h->get_key(bucket), SEED);
    int hash2 = tempHash2 % (h->max);
    int i = 1;

    while (1)
    {
      int newHash = (hash1 + i * hash2) % h->max;
      if (!h->table[newHash])
      {
        h->table[newHash] = (uintptr_t)bucket;
        h->size++;
        break;
      }

      i++;
    }
  }

  return EXIT_SUCCESS;
}

tMunicipio *buscar_hash(tHash *h, char *keyCodigoIbge)
{
  uint32_t tempHash1 = hashf1(keyCodigoIbge, SEED); // Índice inicial
  int hash1 = tempHash1 % (h->max);

  if (h->table[hash1])
  {
    uintptr_t hashValue = h->table[hash1];
    tMunicipio *m = (tMunicipio *)hashValue;

    if (!strcmp(m->codigoIBGE, keyCodigoIbge))
    {
      return m;
    }
  }
  else
  {
    // Colisão, calcula o próximo índice usando a segunda função de hash
    uint32_t tempHash2 = hashf2(keyCodigoIbge, SEED);
    int hash2 = tempHash2 % (h->max);

    for (int i = 1; i < h->size; i++)
    {
      int newHash = (hash1 + i * hash2) % h->max;
      if (h->table[newHash])
      {
        uintptr_t hashValue = h->table[newHash];
        tMunicipio *m = (tMunicipio *)hashValue;

        if (!strcmp(m->codigoIBGE, keyCodigoIbge))
        {
          return m;
        }
      }
    }
  }

  return NULL;
}

tMunicipio *deletar_hash(tHash *h, char *keyCodigoIbge)
{
  uint32_t tempHash1 = hashf1(keyCodigoIbge, SEED); // Índice inicial
  int hash1 = tempHash1 % (h->max);

  if (h->table[hash1])
  {
    uintptr_t hashValue = h->table[hash1];
    tMunicipio *m = (tMunicipio *)hashValue;

    if (!strcmp(m->codigoIBGE, keyCodigoIbge))
    {
      h->table[hash1] = h->deleted;
      h->size -= 1;
      return m;
    }
  }
  else
  {
    // Colisão, calcula o próximo índice usando a segunda função de hash
    int hash2 = hashf2(keyCodigoIbge, SEED);

    for (int i = 1; i < h->size; i++)
    {
      int newHash = (hash1 + i * hash2) % h->max;
      if (h->table[newHash])
      {
        uintptr_t hashValue = h->table[newHash];
        tMunicipio *m = (tMunicipio *)hashValue;

        if (!strcmp(m->codigoIBGE, keyCodigoIbge))
        {
          h->table[hash1] = h->deleted;
          h->size -= 1;
          return m;
        }
      }
    }
  }

  return NULL;
}

int hash_constroi(tHash *h, int nbuckets, char *(*get_key)(char *))
{
  h->table = calloc(sizeof(uintptr_t), nbuckets + 1);
  if (h->table == NULL)
  {
    return EXIT_FAILURE;
  }
  h->max = nbuckets + 1;
  h->size = 0;
  h->deleted = 0;
  h->get_key = get_key;
  return EXIT_SUCCESS;
}

void hash_apaga(tHash *h)
{
  int hash;
  for (hash = 0; hash < h->max; hash++)
  {
    if (h->table[hash] != 0)
    {
      if (h->table[hash])
      {
        free((void *)h->table[hash]);
      }
    }
  }
  free(h->table);
}

void lerArquivoM(tHash *h)
{
  FILE *arquivoM;

  arquivoM = fopen("municipios.txt", "r");

  if (!arquivoM)
  {
    printf("Erro ao abrir o arquivo de municípios\n");
  }
  int x = 9;
  while (x == 9)
  {
    tMunicipio *m = malloc(sizeof(tMunicipio));
    x = fscanf(arquivoM, " %s %[^\t] %f %f %d %d %d %d %[^\n]\n", m->codigoIBGE,
               m->nome, &m->latitude, &m->longitude, &m->capital, &m->codigoUf,
               &m->siafi, &m->ddd, m->fusoHorario);
    inserir_hash(h, aloca_municipio(m->codigoIBGE, m->nome, m->latitude,
                                    m->latitude, m->capital, m->codigoUf,
                                    m->siafi, m->ddd, m->fusoHorario));
  }

  return;
}

void test_buscar()
{
  tHash h;
  int nbuckets = 10;
  tMunicipio *mu;
  hash_constroi(&h, nbuckets, get_key);

  assert(inserir_hash(&h, aloca_municipio("5200050", "Abadia de Goiás", -16.7573, -49.4412, 0, 52, 1050, 62, "America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100104",	"Abadia dos Dourados",	-18.4831,	-47.3916,	0,	31,	4001,	34,	"America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("5200100",	"Abadiânia",	-16.197,	-48.7057,	0,	52,	9201,	62,	"America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100203",	"Abaeté",	-19.1551,	-45.4444,	0,	31,	4003,	37,	"America/Sao_Paulo")) == EXIT_SUCCESS);

  mu = buscar_hash(&h, "5200100");
  assert(!strcmp(mu->codigoIBGE, "5200100"));

  hash_apaga(&h);
}

void test_remover()
{
  tHash h;
  tMunicipio *mu;
  int nbuckets = 10;
  hash_constroi(&h, nbuckets, get_key);

  assert(inserir_hash(&h, aloca_municipio("5200050", "Abadia de Goiás", -16.7573, -49.4412, 0, 52, 1050, 62, "America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100104",	"Abadia dos Dourados",	-18.4831,	-47.3916,	0,	31,	4001,	34,	"America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("5200100",	"Abadiânia",	-16.197,	-48.7057,	0,	52,	9201,	62,	"America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100203",	"Abaeté",	-19.1551,	-45.4444,	0,	31,	4003,	37,	"America/Sao_Paulo")) == EXIT_SUCCESS);

  assert(h.size == 4);
  assert(deletar_hash(&h, "5200100"));
  mu = buscar_hash(&h, "5200100");
  assert(mu == NULL);
  assert(h.size == 3);

  hash_apaga(&h);
}

void test_inserir()
{
  tHash h;
  tMunicipio *mu;
  int nbuckets = 10;
  hash_constroi(&h, nbuckets, get_key);

  assert(inserir_hash(&h, aloca_municipio("5200050", "Abadia de Goiás", -16.7573, -49.4412, 0, 52, 1050, 62, "America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100104",	"Abadia dos Dourados",	-18.4831,	-47.3916,	0,	31,	4001,	34,	"America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("5200100",	"Abadiânia",	-16.197,	-48.7057,	0,	52,	9201,	62,	"America/Sao_Paulo")) == EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100203",	"Abaeté",	-19.1551,	-45.4444,	0,	31,	4003,	37,	"America/Sao_Paulo")) == EXIT_SUCCESS);

  hash_apaga(&h);
}

int main(int argc, char *argv[])
{
  int op;
  char codigoIBGE[50];
  tHash h;
  tMunicipio *mu;
  int nbuckets = 5570;
  hash_constroi(&h, nbuckets, get_key);

  lerArquivoM(&h);

  do
  {
    printf("Escolha uma opção\n");
    printf("0 - Encerrar\n1 - Buscar município\n2 - Deletar município\n");
    scanf("%d", &op);

    if (op == 1)
    {
      printf("Informe o código do IBGE\n");
      scanf(" %s", codigoIBGE);
      tMunicipio *m = buscar_hash(&h, codigoIBGE);
      printf("////////////////////////////////////////////\n");
      printf("Código IBGE: %s\n", m->codigoIBGE);
      printf("Nome: %s\n", m->nome);
      printf("Código UF: %d\n", m->codigoUf);
      printf("Capital: %d\n", m->capital);
      printf("Latitude: %f\n", m->latitude);
      printf("Longitude: %f\n", m->longitude);
      printf("Siafi: %d\n", m->siafi);
      printf("DDD: %d\n", m->ddd);
      printf("Fuso Horário: %s\n", m->fusoHorario);
      printf("////////////////////////////////////////////\n");
    }
    else if (op == 2)
    {
      printf("Informe o código do IBGE\n");
      scanf(" %s", codigoIBGE);
      tMunicipio *m = deletar_hash(&h, codigoIBGE);
      if (m)
      {
        printf("////////////////////////////////////////////\n");
        printf("Hash deletado\n");
        printf("////////////////////////////////////////////\n");
      }
    }
  } while (op != 0);

  hash_apaga(&h);

  test_inserir();
  test_buscar();
  test_remover();

  return 0;
}
