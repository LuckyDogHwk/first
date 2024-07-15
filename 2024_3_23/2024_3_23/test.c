#define _CRT_SECURE_NO_WARNINGS 1


#include<stdio.h>
#include<string.h>
#include<math.h>






//int main()
//{
//	char arr[10001] = {0};
//
//	gets(arr);
//	int left = 0;
//	int right = strlen(arr)-1;
//	while (left < right)
//	{
//		char tmp = arr[left];
//		arr[left] = arr[right];
//		arr[right] = tmp;
//		left++;
//		right--;
//	}
//	printf("%s\n",arr);
//
//	return 0;
//}






//int pownum(int a, int num)
//{
//	int s = num;
//	int sum = 0;
//	int i = 0;
//	for (i = 0; i <= num; i++)
//	{
//		sum += a*pow(10,i);
//	}
//	if (s == 0)
//	{
//		return a;
//	}
//	s--;
//	return sum + pownum(a,s);
//}
//
//int main()
//{
//	int a = 0;
//	int n = 5;
//	int i = 0;
//	int sum = 0;
//	int k = 0;
//
//	scanf("%d",&a);
//	sum = pownum(a,n - 1);
//	for (i = 0; i < n; i++)
//	{
//		k = k * 10 + a;
//		sum += k;
//	}
//
//	printf("%d\n",sum);
//
//	return 0;
//}





//int main()
//{
//	int i = 0;
//
//	for (i = 0; i <= 100000; i++)
//	{
//		int tmp = i;
//		int n = 0;
//		int sum = 0;
//		while (tmp)
//		{
//			n++;
//			tmp /= 10;
//		}
//		tmp = i;
//		while (tmp)
//		{
//			sum += pow(tmp % 10, n);
//			tmp /= 10;
//		}
//		if (sum == i)
//		{
//			printf("%d ",i);
//		}
//	}
//
//	return 0;
//}





