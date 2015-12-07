void hanoi(string a, string b, string c, int n);

int main() {
	int n;
	out << "输入盘子个数：";
	in >> n;
	hanoi("A", "B", "C", n);
	return 0;
}

void hanoi(string a, string b, string c, int n) {
	if(n == 0) {
		return;
	} else {
	  hanoi(a, c, b, n - 1);
	  out << "Move " + n + ":\t[" + a + " --> " + c + "]\n";
	  hanoi(b, a, c, n - 1);
  }
}
