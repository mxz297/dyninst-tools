static int x = 10;

int foo(int x) {
    return x * x;
}

int zero() {
    return 0;
}


void bar(int y) {
    int buf[10];
    int i;
    for (i = 0 ; i < 100; ++i)
        buf[i] = i;   
}

int main() {
    int y = foo(x);
    bar(y);
}
