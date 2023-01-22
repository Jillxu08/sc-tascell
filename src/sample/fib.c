
int printf (char const *, ...);

int atoi (char const *);

int
fib (int n)
{
  if (n <= 2)
    return 1;
  else
    return fib (n - 2) + fib (n - 1);
}

int
main (int argc, char **argv)
{
  int a = 22;
  if (argc > 1)
    a = atoi (argv[1]);
  else;
  printf ("fib(%d) = %d\n", a, fib (a));
  return 0;
}
