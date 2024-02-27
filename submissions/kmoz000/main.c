#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CITY_NAME_LEN 20
#define MAX_PRODUCT_NAME_LEN 20
#define MAX_PRICE 101.0
#define TABLE_SIZE 101

typedef struct
{
    char name[MAX_CITY_NAME_LEN];
    double totalPrices;
} City;

typedef struct
{
    City *data;
    int occupied;
} HashEntry;

typedef struct
{
    HashEntry table[TABLE_SIZE];
} HashTable;

typedef struct
{
    char city[MAX_CITY_NAME_LEN];
    char product[MAX_PRODUCT_NAME_LEN];
    double price;
} ProductEntry;

typedef struct
{
    ProductEntry entries[5];
    int size;
} CheapestFive;

int compareProductEntry(const void *a, const void *b)
{
    double diff = ((ProductEntry *)a)->price - ((ProductEntry *)b)->price;
    return (diff > 0) ? (1) : (-1);
}
void updateCheapestSix(CheapestFive *cheapestSix, ProductEntry *newEntry)
{
    qsort(cheapestSix->entries, cheapestSix->size, sizeof(ProductEntry), compareProductEntry);
    if (newEntry->price < cheapestSix->entries[cheapestSix->size - 1].price)
    {
        cheapestSix->entries[cheapestSix->size - 1] = *newEntry;
    }
}

unsigned int hash(char *str)
{
    unsigned int hash = 0;
    while (*str)
    {
        hash = (hash * 31) + (*str++);
    }
    return hash % TABLE_SIZE;
}

void initializeHashTable(HashTable *ht)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        ht->table[i].data = NULL;
        ht->table[i].occupied = 0;
    }
}

int has(HashTable *ht, char *city)
{
    unsigned int index = hash(city);
    if (ht->table[index].occupied && strcmp(ht->table[index].data->name, city) == 0)
    {
        return 1;
    }
    return 0;
}

void addToTotal(HashTable *ht, char *city, double price)
{
    unsigned int index = hash(city);
    if (has(ht, city))
    {
        if (ht->table[index].data != NULL)
        {
            ht->table[index].data->totalPrices += price;
        }
    }
    else
    {
        City *newCity = (City *)malloc(sizeof(City));
        if (newCity == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        strcpy(newCity->name, city);
        newCity->totalPrices = price;
        ht->table[index].data = newCity;
        ht->table[index].occupied = 1;
    }
}
CheapestFive newFiveCheapProducts()
{
    CheapestFive cheapestProducts;
    ProductEntry initialEntries[] = {
        {"", "", MAX_PRICE},
        {"", "", MAX_PRICE},
        {"", "", MAX_PRICE},
        {"", "", MAX_PRICE},
        {"", "", MAX_PRICE}};

    for (int i = 0; i < 5; i++)
    {
        cheapestProducts.entries[i] = initialEntries[i];
    }
    return cheapestProducts;
}
typedef struct
{
    FILE *file;
    HashTable *cityHashTable;
    CheapestFive *cheapestProducts;
} ThreadData;

void *readDataThread(void *arg)
{
    ThreadData *threadData = (ThreadData *)arg;

    ProductEntry newTempData;
    while (fscanf(threadData->file, "%49[^,],%49[^,],%lf\n", newTempData.city, newTempData.product, &newTempData.price) == 3)
    {
        addToTotal(threadData->cityHashTable, newTempData.city, newTempData.price);
        updateCheapestSix(threadData->cheapestProducts, &newTempData);
    }

    pthread_exit(NULL);
}
void readDataFromFile(HashTable *cityHashTable, CheapestFive *cheapestProducts, int numThreads)
{
    FILE *file = fopen("../../input.txt", "r");
    if (!file)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];

    for (int i = 0; i < numThreads; i++)
    {
        threadData[i].file = file;
        threadData[i].cityHashTable = cityHashTable;
        threadData[i].cheapestProducts = cheapestProducts;

        if (pthread_create(&threads[i], NULL, readDataThread, &threadData[i]) != 0)
        {
            perror("Thread creation failed");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    fclose(file);
}
int main()
{
    CheapestFive cheapestProducts = newFiveCheapProducts();
    HashTable cityHashTable;
    initializeHashTable(&cityHashTable);
    cheapestProducts.size = 5;
    int numThreads = 4;
    readDataFromFile(&cityHashTable, &cheapestProducts, numThreads);
    for (int i = 0; i < cheapestProducts.size; i++)
    {
        if (strlen(cheapestProducts.entries[i].city) > 0)
        {
            printf("%s, %s, %.2f\n", cheapestProducts.entries[i].city, cheapestProducts.entries[i].product, cheapestProducts.entries[i].price);
        }
    }
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        if (cityHashTable.table[i].occupied)
        {
            free(cityHashTable.table[i].data);
        }
    }
    return 0;
}
