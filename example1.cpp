#include"lang/serializable.h"
#include<iostream>
#include<fstream>
#include<cstring>
using namespace std;
struct Node:public Serializable 
{
	Node()
	{
		std::memset(z,0,sizeof(z));
		t={1,2,3,4};
	}
	Config get_config()const 
	{
		Config config=Serializable::get_config(this);
		config.update({
			{"x",x},
			{"y",y},
			{"z",z},
			{"t",t}
		});
		return config;
	}
	tuple<float,std::pair<int,std::string>>x;
	std::vector<Node*>y;
	int z[3];
	std::array<int,4>t;
};
int main()
{
	Serializable::Regist<Node>();                                                   //ע��
	Node a=*(Node*)Reflectable::get_instance("Node");                               //����ʵ��
	Reflectable::set_field(a,"x",make_tuple(3.2f,make_pair(5,string{"test"})));     //ͨ�������޸�����
	Reflectable::set_field(a,"y",std::vector<Node*>{new Node,nullptr}); 
	int*z=(int*)Reflectable::get_field(a,"z");                                      //������ԣ��������޸�
	z[0]=2021,z[1]=10,z[2]=18;
	a.t[0]=2021,a.t[1]=10,a.t[2]=19,a.t[3]=10;
	std::string json=Serializable::dumps(a);                                        //���л�Ϊjson��ʽ���ַ���
	cout<<"json\n"<<json<<endl;         
	Node&b=Serializable::loads<Node>(json);                                         //ͨ��json��ʽ���ַ������з����л�
	cout<<endl<<a.get_config().serialized_to_string(true);
	cout<<endl<<b.get_config().serialized_to_string(true);                          //��ӡ���
} 
