#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__
#include<iostream>
#include<functional>
#include<algorithm>
#include"reflectable.h"
class Serializable:public Reflectable
{
public: 
	virtual ~Serializable(){};
	template<typename T,typename ...Args>
	struct Regist;
	template<typename T>
	struct Regist<T>;
	template<typename Object>
	Config get_config(const Object*object)const; //Reflectable::get_config
	template<typename Object>
	static std::string dumps(const Object&object); //���л�����
	template<typename Object=void*>
	static auto loads(const std::string&json);     //�����л���ԭ����
	static std::unordered_map<std::string,std::function<void(void*,const std::string&)>>from_config_string;//��ͬ���ͷ����л�������
	static Config decode(const std::string&serialized);                              
	void from_config(Config&config); //��Config�л�ԭԭʼ����
};
std::unordered_map<std::string,std::function<void(void*,const std::string&)>>Serializable::from_config_string;
template<typename Object>
Config Serializable::get_config(const Object*object)const
{
	return Reflectable::get_config(object);
}
void Serializable::from_config(Config&config)
{
	std::string&class_name=config["class_name"];
	class_name.erase(                                        
		std::remove_if(class_name.begin(),class_name.end(),[](auto ch){return ch=='\"';}),//ȥ�����ߵ�����
		class_name.end());
	for(auto&it:config)
	{
		if(it.first!="class_name")
		{
			auto&field_name=it.first;
			auto&value=it.second;
			std::string&type=Reflectable::get_field_type(class_name,field_name);     //�õ���������
			void*field=Reflectable::get_field(this,class_name,field_name);           //�õ����Ե�ַ
			if(type[type.size()-1]=='*'&&value=="null")                              //��ָ��
				*(void**)field=nullptr;
			else if(value[0]=='{') //struct
				Serializable::from_config_string[type](field,value); //������һ��
			else if(value[0]=='[') //list
				ConfigPair::from_config_string[type](field,value);
			else 
				ConfigPair::from_config_string[type](field,value);                    //�޸�ֵ
		}
	}
}
template<typename Object>
std::string Serializable::dumps(const Object&object)
{
	return object.get_config().serialized_to_string();
}
Config Serializable::decode(const std::string&serialized) //��ª������״̬�Զ�����
{
	constexpr int init=0;                                                          //�������״̬
	constexpr int parse_value=1;
	constexpr int parse_struct=2;
	constexpr int parse_fundamental=3;
	constexpr int parse_iterable=4;
	constexpr int parse_string=5;
	constexpr int end_parse=6;
	std::string key;
	std::string value;
	int nested_struct_layer=0;
	int nested_iterable_layer=0;
	int state=init;
	int length=serialized.size();
	Config config;
	for(int i=0;i<length;++i)
	{
		auto&it=serialized[i];
		if(state==init)                                        //��ð����ǰ���ַ�Ϊ������
		{
			if(it==':')
				state=parse_value;                             //ð���Ժ��������ֵ��Ӧ���ַ���
			else if(it!='\"'&&it!='{'&&it!=','&&it!=' ')       //���ǵ��ų����ߵ�˫����
				key.push_back(it);
		}
		else if(state==parse_value) //��ʼ�������
		{
			if(it=='{')                                        //����Ǵ����Ű������ģ��Ǿ���struct����
			{
				value.push_back(it);
				nested_struct_layer++;  //{{{}}}Ƕ��,��������ż�1���ִ�����-1,��0�����
				state=parse_struct;
			}
			else if(it=='[')
			{
				value.push_back(it);
				nested_iterable_layer++; 
				state=parse_iterable;
			}
			else if(it=='\"')
			{
				value.push_back(it);
				state=parse_string;
			}
			else if(it!=' ')                                        //������ǻ�������
			{
				value.push_back(it);
				state=parse_fundamental;
			}
		}
		else if(state==parse_string)
		{
			value.push_back(it);
			if(it=='\"'&&serialized[i-1]!='\\') // \" ת���ַ����ǽ���.
			{
				state=end_parse;
				--i;
			}
		}
		else if(state==parse_fundamental) 
		{
			if(it==','||it=='}'||it=='\"') //�������Ž���,�������һ�����ԣ������Ļ��Ǵ����ţ������ַ���������˫����
			{
				if(it=='\"')
					value.push_back(it);  //˫������Ҫ���ֵ����
				state=end_parse;
				--i;
				continue;
			}
			value.push_back(it);
		}
		else if(state==parse_iterable) //������Ƕ�׵����
		{
			if(it==']'||it=='[')
			{
				nested_iterable_layer+=(it==']'?-1:1);
				value.push_back(it);
				if(nested_iterable_layer==0)
				{
					state=end_parse;
					--i;
				}
				continue;
			}
			value.push_back(it);
		}
		else if(state==parse_struct)      //���������Ž���
		{
			if(it=='}'||it=='{')
			{
				nested_struct_layer+=(it=='}'?-1:1);
				value.push_back(it);
				if(nested_struct_layer==0)
				{
					state=end_parse;
					--i;
				}
				continue;
			}
			value.push_back(it);
		}
		else if(state==end_parse)        //������һ�����Զ�Ӧ�ļ�ֵ�ԣ���¼��config��
		{
			state=init;
			config[key]=value;
//			std::cout<<"<key>:"<<key<<"   <value>:"<<value<<std::endl;
			key.clear();
			value.clear();
		}
	}
	return config;
}
template<typename Object=void*>
auto Serializable::loads(const std::string&json)
{
	Config config=Serializable::decode(json);                //��json�ַ�����ԭConfig
	std::string&class_name=config["class_name"];
	class_name.erase(                                        
		std::remove_if(class_name.begin(),class_name.end(),[](auto ch){return ch=='\"';}),//ȥ�����ߵ�����
		class_name.end());																																																																																																																																																
	void*object=Reflectable::get_instance(class_name);        //����ʵ��
	Serializable::from_config_string[class_name](object,json);//�����л���ԭ
	if constexpr(std::is_same<Object,void*>::value)
		return object;
	else
		return std::ref(*(Object*)object);
}
template<typename T,typename ...Args>
struct Serializable::Regist
{
	Regist()
	{
		using Tptr=T*;
		from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void    //ʵ�ʻ��ǵ���from_config���еݹ�.
		{
			Config config=Serializable::decode(value);
			(*(T*)object).from_config(config);
		};
		from_config_string[GET_TYPE_NAME(Tptr)]=[](void*object,const std::string&value)->void //��Ӧ��ָ��
		{
			Config config=Serializable::decode(value);
			if(value=="null")
			{
				(*(Tptr*)object)=nullptr;
			}
			else
			{
		   		(*(Tptr*)object)=(T*)Reflectable::get_instance(GET_TYPE_NAME(T));
				(*(Tptr*)object)->from_config(config);
			}
		};
		Reflectable::Regist<T>();
		Serializable::Regist<Args...>();
	}
};
template<typename T>
struct Serializable::Regist<T>
{
	Regist()
	{
		using Tptr=T*;
		from_config_string[GET_TYPE_NAME(T)]=[](void*object,const std::string&value)->void   //ʵ�ʻ��ǵ���from_config���еݹ�.
		{
			Config config=Serializable::decode(value);
			(*(T*)object).from_config(config);
		};
		from_config_string[GET_TYPE_NAME(Tptr)]=[](void*object,const std::string&value)->void //��Ӧ��ָ��
		{
			Config config=Serializable::decode(value);
			if(value=="null")
			{
				(*(Tptr*)object)=nullptr;
			}
			else
			{
		   		(*(Tptr*)object)=(T*)Reflectable::get_instance(GET_TYPE_NAME(T));
				(*(Tptr*)object)->from_config(config);
			}
		};
		Reflectable::Regist<T>();
	}
};
#endif
