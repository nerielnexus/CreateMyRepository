#include <iostream>
#include <fstream>

struct insa
{
	char name[ 20 ];
	int age;
	char address[ 20 ];
};

int main( void )
{
	insa man[5] = 
	{
		{ "������", 20, "����" },
		{ "������", 30, "�λ�" },
		{ "������", 24, "�뱸" },
		{ "������", 34, "����" },
		{ "ä����", 25, "����" }
	};

	std::ofstream fout;
	fout.open( "insa.txt",std::ofstream::app );

	for( int i = 0; i < 5; i++ )
		fout << man[ i ].name << " " << man[ i ].age << " " << man[ i ].address << std::endl;

	fout.close( );
}