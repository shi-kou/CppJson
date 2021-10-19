#ifndef __CONFIG_H__
#define __CONFIG_H__
#include"configpair.h"
#include<unordered_map>
class Config//������һ����ֵ�ԣ�����ÿһ�����͵�һ�ε��õ�ʱ����������ʱ��һЩ������ص���Ϣ 
{            //***field_infoӦ����Serializable��������������ﲻӦ��ɾ��ָ��*** 
public:
	Config():field_info(nullptr){}
	template<typename T>
	Config(std::unordered_map<std::string,std::pair<std::string,std::size_t>>*field_info,T*object);
	//�ڵ�һ�α����õ�ʱ����Ҫȥ����������Ϣ,field_info��¼��������->(��������,��ַƫ����),��¼�����ں��������л�����Ҫ�õ� 
	std::string serialized_to_string(bool first_nested_layer=false)const;      //���л�Ϊ�ַ���  
	std::string&operator[](const std::string&key)const; //��ֵ�� 
	std::string&operator[](std::string&key);  
	void update(const std::initializer_list<ConfigPair>&pairs);
	//��ӱ�����,��ʱ��ConfigPair�л��¼��������Ϣ������������ʱ�����÷����л�ʱ����Ҫ��Ӧ�Ļ�ԭ����;
	//config.update({{"namea",this->name1},{"name2",this->name2}});
	auto begin();
	auto end();
private:
	mutable std::unordered_map<std::string,std::string>config;
	std::unordered_map<std::string,std::pair<std::string,std::size_t>>*field_info;
	std::size_t class_header_address;
	std::size_t class_size;
};
std::ostream operator<<(std::ostream&os,Config&config);
template<typename T>
Config::Config(std::unordered_map<std::string,std::pair<std::string,std::size_t>>*field_info,T*object): //�ڵ�һ�α����õ�ʱ����Ҫȥ����������Ϣ 
	field_info(field_info),                                                                             //Ϊ����ķ����л���׼�� 
	class_header_address((std::size_t)(object)),                                                        
	class_size(sizeof(T)){}
void Config::update(const std::initializer_list<ConfigPair>&pairs)//��ӱ�����,config.update({{"namea",this->name1},{"name2",this->name2}});
{
	for(auto&it:pairs)
	{
		config[it.key]=it.value;
		if(field_info!=nullptr)                                                 //ֻ������һ��
		{
			(*field_info)[it.key].first=it.type;                     //����
			(*field_info)[it.key].second=it.address-this->class_header_address; //��ַƫ�������������ʳ�Ա���� 
		}
	}
}
std::string&Config::operator[](const std::string&key)const //��ֵ�� 
{
	return config[key];
}
std::string&Config::operator[](std::string&key) //��ֵ�� 
{
	return config[key];
}
std::string Config::serialized_to_string(bool first_nested_layer)const //���л���
{
	std::ostringstream oss;
	char end=first_nested_layer?'\n':' ';
	oss<<"{"<<end;
	for(auto it=config.begin(),next=++config.begin();it!=config.end();++it,next!=config.end()?++next:next)
	{
		if(it!=config.end()&&next==config.end()) //���һ��Ԫ�أ�û�ж���
			oss<<"\""<<(*it).first<<"\":"<<(*it).second<<end;
		else
			oss<<"\""<<(*it).first<<"\":"<<(*it).second<<","<<end;
	}
	oss<<"}"<<end;
	return oss.str();
}
auto Config::begin()
{
	return config.begin();
}
auto Config::end()
{
	return config.end();
}
#endif
