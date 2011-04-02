sum = 0;
result = "";

for (i = 0; i <= 100 ; i++)
{
	result += i + "+"; 
	sum +=i;
	if(!(i % 10 ))
		result += "\n\n";
	yield();
}

result += i +  "=" + sum ;

result;

