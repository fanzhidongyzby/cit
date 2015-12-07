int fun()
{
	int a = 1, b = a + 2, c = a * b, d = b * c, e = 10;
	int i = 10;

	if(a > b) {
		c = a * b;
		d = b / c;
		e = c % d;
	}

	do {
		c = a * b;
		d = b * c;
		e = c * d;
	} while(i--);

	return a + e;
}

int main() {
	int i = 500, j;

	while(i--){
		j = fun();
	}

	return i = j;
}
