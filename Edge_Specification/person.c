#include <stdio.h>

typedef struct clothes {
    char color[10];
    int length;
} Clothes;

typedef struct person_t {
    int age;
    char name[10];
    Clothes *s;
} Person;

Clothes clo = {"yellow", 2};

Person p = {10, "bob", &clo};

Person* f(Person *p1) {
    p1->age++;
    p.age++;
    return p1;
}

int main() {
    Clothes c = {"red", 5};
    Person p1 = {10, "Jack", &c};
    Person *pt = &p1;
    Person *p3 = f(pt);
    Person *p4 = f(&p);
    p3->age++;
    p4->age++;
    return 0;
}