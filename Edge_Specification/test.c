#include <stdio.h>
#include <stdlib.h>

typedef struct clothes {
    char color[10];
    int length;
} Clothes;

typedef struct person_t {
    int age;
    char name[10];
    Clothes *s;
} Person;

// Clothes clo = {"yello", 1};

void f(Person *p1) {
    char *name = p1->name;
    char *color = p1->s->color;
    p1->age = 10;
    printf("%s is wearing %s today.", name, color);
}
// void test(char *buf) {
//     printf("%s\n", buf);
//     strcat(buf, "world");
// }

int main() {
    Clothes c = {"red", 5};
    Person p = {10, "Jack", &c};
    Person *pt = &p;
    // Clothes c2 = clo;
    // if (c.length >= 5) {
    //     p.age = 10;
    // } else {
    //     p.age = 4;
    // }
    f(pt);
    // Clothes *c1 = &clo;
    // c.length += c1->length;
    // Person *t1;
    // t1->s = &clo;
    // printf("%s", c2.color);
    // char *buffer = malloc(100);
    // memset(buffer, 'b', 7);
    // memcpy(buffer, "hello", 6);
    // test(buffer);

    return 0;
}