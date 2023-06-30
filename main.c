#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SEED 0x12345678

typedef struct {
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
                      int ddd, char *fusoHorario) {
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

typedef struct {
  uintptr_t *table;
  int size;
  int max;
  uintptr_t deleted;
  char *(*get_key)(void *);
} tHash;

// Função de hash 1
uint32_t hashf1(const char *str, uint32_t h) {
  /* One-byte-at-a-time Murmur hash
  Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
  for (; *str; ++str) {
    h ^= *str;
    h *= 0x12345678;
    h ^= h >> 15;
  }
  return h;
}

uint32_t hashf2(const char *str, uint32_t h) {
  /* One-byte-at-a-time Murmur hash
  Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
  for (; *str; ++str) {
    h ^= *str;
    h *= 0x2bd1e995;
    h ^= h >> 15;
  }
  return h;
}

int inserir_hash(tHash *h, void *bucket) {
  uint32_t tempHash1 = hashf1(h->get_key(bucket), SEED); // Índice inicial
  int hash1 = tempHash1 % (h->max);
  if (h->max == (h->size + 1)) {
    free(bucket);
    return EXIT_FAILURE;
  }

  if (!h->table[hash1]) {
    h->table[hash1] = (uintptr_t)bucket; // Espaço vazio, insere diretamente
    h->size++;
  } else {
    // Colisão, calcula o próximo índice usando a segunda função de hash
    uint32_t tempHash2 = hashf2(h->get_key(bucket), SEED);
    int hash2 = tempHash2 % (h->max);
    int i = 1;

    while (1) {
      int newHash = (hash1 + i * hash2) % h->max;
      if (!h->table[newHash]) {
        h->table[newHash] = (uintptr_t)bucket;
        h->size++;
        break;
      }

      i++;
    }
  }

  return EXIT_SUCCESS;
}

int hash_constroi(tHash *h, int nbuckets, char *(*get_key)(char *)) {
  h->table = calloc(sizeof(uintptr_t), nbuckets + 1);
  if (h->table == NULL) {
    return EXIT_FAILURE;
  }
  h->max = nbuckets + 1;
  h->size = 0;
  h->deleted = NULL;
  h->get_key = get_key;
  return EXIT_SUCCESS;
}

tMunicipio *buscar_hash(tHash *h, char *keyCodigoIbge) {
  uint32_t tempHash1 = hashf1(keyCodigoIbge, SEED); // Índice inicial
  int hash1 = tempHash1 % (h->max);

  if (h->table[hash1]) {
    uintptr_t hashValue = h->table[hash1];
    tMunicipio *m = (tMunicipio *)hashValue;

    if (!strcmp(m->codigoIBGE, keyCodigoIbge)) {
      return m;
    }
  } else {
    // Colisão, calcula o próximo índice usando a segunda função de hash
    uint32_t tempHash2 = hashf2(keyCodigoIbge, SEED);
    int hash2 = tempHash2 % (h->max);

    for (int i = 1; i < h->size; i++) {
      int newHash = (hash1 + i * hash2) % h->max;
      if (h->table[newHash]) {
        uintptr_t hashValue = h->table[newHash];
        tMunicipio *m = (tMunicipio *)hashValue;

        if (!strcmp(m->codigoIBGE, keyCodigoIbge)) {
          return m;
        }
      }
    }
  }

  return NULL;
}

tMunicipio *deletar_hash(tHash *h, char *keyCodigoIbge) {
  uint32_t tempHash1 = hashf1(keyCodigoIbge, SEED); // Índice inicial
  int hash1 = tempHash1 % (h->max);

  if (h->table[hash1]) {
    uintptr_t hashValue = h->table[hash1];
    tMunicipio *m = (tMunicipio *)hashValue;

    if (!strcmp(m->codigoIBGE, keyCodigoIbge)) {
      h->table[hash1] = h->deleted;
      h->size -= 1;
      return m;
    }
  } else {
    // Colisão, calcula o próximo índice usando a segunda função de hash
    int hash2 = hashf2(keyCodigoIbge, SEED);

    for (int i = 1; i < h->size; i++) {
      int newHash = (hash1 + i * hash2) % h->max;
      if (h->table[newHash]) {
        uintptr_t hashValue = h->table[newHash];
        tMunicipio *m = (tMunicipio *)hashValue;

        if (!strcmp(m->codigoIBGE, keyCodigoIbge)) {
          h->table[hash1] = h->deleted;
          h->size -= 1;
          return m;
        }
      }
    }
  }

  return NULL;
}

void hash_apaga(tHash *h) {
  int hash;
  for (hash = 0; hash < h->max; hash++) {
    if (h->table[hash] != 0) {
      if (h->table[hash]) {
        free((void *)h->table[hash]);
      }
    }
  }
  free(h->table);
}

void lerArquivoM(tHash *h) {
  FILE *arquivoM;

  arquivoM = fopen("municipios.txt", "r");

  if (!arquivoM) {
    printf("Erro ao abrir o arquivo de municípios\n");
  }
  int x = 9;
  while (x == 9) {
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

void test_buscar() {
  tHash h;
  int nbuckets = 10;
  tMunicipio *mu;
  hash_constroi(&h, nbuckets, get_key);

  assert(inserir_hash(&h, aloca_municipio("5200050", "Abadia de Goiás",
                                          -16.7573, -49.4412, 0, 52, 1050, 62,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100104", "Golden City", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("31001077", "Golden City", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("45001079", "Big Field", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio(
                              "31001077", "CPO", -18.4831, -47.3916, 0, 31,
                              4001, 34, "America/Sao_Paulo")) == EXIT_SUCCESS);

  mu = buscar_hash(&h, "45001079");
  assert(!strcmp(mu->codigoIBGE, "45001079"));

  hash_apaga(&h);
}

void test_remover() {
  tHash h;
  tMunicipio *mu;
  int nbuckets = 10;
  hash_constroi(&h, nbuckets, get_key);

  assert(inserir_hash(&h, aloca_municipio("5200050", "Abadia de Goiás",
                                          -16.7573, -49.4412, 0, 52, 1050, 62,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100104", "Golden City", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("31001077", "Golden City", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("45001079", "Big Field", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio(
                              "31001077", "CPO", -18.4831, -47.3916, 0, 31,
                              4001, 34, "America/Sao_Paulo")) == EXIT_SUCCESS);

  assert(h.size == 5);
  assert(deletar_hash(&h, "45001079"));
  mu = buscar_hash(&h, "45001079");
  assert(mu == NULL);

  hash_apaga(&h);
}

void test_inserir() {
  tHash h;
  tMunicipio *mu;
  int nbuckets = 10;
  hash_constroi(&h, nbuckets, get_key);

  assert(inserir_hash(&h, aloca_municipio("5200050", "Abadia de Goiás",
                                          -16.7573, -49.4412, 0, 52, 1050, 62,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("3100104", "Golden City", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("31001077", "Golden City", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio("45001079", "Big Field", -18.4831,
                                          -47.3916, 0, 31, 4001, 34,
                                          "America/Sao_Paulo")) ==
         EXIT_SUCCESS);
  assert(inserir_hash(&h, aloca_municipio(
                              "31001077", "CPO", -18.4831, -47.3916, 0, 31,
                              4001, 34, "America/Sao_Paulo")) == EXIT_SUCCESS);

  hash_apaga(&h);
}

int main(int argc, char *argv[]) {
  tHash h;
  tMunicipio *mu;
  int nbuckets = 5570;
  hash_constroi(&h, nbuckets, get_key);

  lerArquivoM(&h);

  tMunicipio *m = buscar_hash(&h, "1500206");

  printf("%s", m->nome);
  // test_inserir();
  // test_buscar();
  // test_remover();

  return 0;
}
