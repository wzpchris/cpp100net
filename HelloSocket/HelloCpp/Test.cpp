#include <stdio.h>
#include <functional>

int funcA(int n, int m) {
	printf("funcA\n");
	return 0;
}

int main() {
	/*std::function<int(int, int)> call = funcA;
	call(4, 5);*/

	std::function<int(int, int)> call;
	int n = 5;
	call = [&n/*外部变量捕获列表*/](/*参数列表*/int a, int b) -> int /*返回类型*/
	{
		//函数体
		printf("lamba n=%d\n", n);
		return n + a + b;
	};
	int f = call(1, 2);
	printf("n=%d\n", n);
	printf("f=%d\n", f);

	getchar();
	return 0;
}
