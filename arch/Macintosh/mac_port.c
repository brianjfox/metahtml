/* mac_port.c: Specific functions required on the mac. */

int
flock (int fd, int how)
{
	return (10);
}

long
random (void)
{
   return (rand ());
}

void
srandom (unsigned seed)
{
	srand (seed);
}

char *
initstate (unsigned seed, char *state, int n)
{
	return ((char *)0);
}

char *
setstate (char *newstate)
{
	return ((char *)0);
}

SpinCursor (void)
{
}