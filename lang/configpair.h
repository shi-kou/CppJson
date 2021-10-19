#ifndef __CONFIGPAIR_H__
#define __CONFIGPAIR_H__
#include<string>
#include<typeinfo>
#include<sstream>
#include<vector>
#include<cxxabi.h>
#include<unordered_map>
#include<functional>
#include"utils.h"
#define GET_TYPE_NAME(type)\
abi::__cxa_demangle(typeid(type).name(),0,0,0)
struct ConfigPair
{
	template<typename T>
	ConfigPair(const std::string&name,const T&object);
	std::string key;    //��Ա�������ƣ������������ƶ�Ӧ 
	std::string value;  //��Ա������ֵת��Ϊ�ַ����Ľ��(����ǻ������ͣ���ֱ��ת��Ϊ�ַ���������ΪǶ���ֵ�ṹ) 
	std::string type;   //����,GET_TYPE_NAME(T),�൱�ڼ�¼�������ˣ������л����õ� 
	std::size_t address;//��ַ�����潫���������Ա������ַƫ�����������л���ʱ��ͨ��(void*)(�����ַ+ƫ����)�����ʳ�Ա���� 
	template<typename Object>
	static std::string get_config_string(const Object&object);
	static std::unordered_map<std::string,std::function<void(void*,const std::string&)>>from_config_string; //���ַ����л�ԭ����
};
std::unordered_map<std::string,std::function<void(void*,const std::string&)>>ConfigPair::from_config_string; //���ַ����л�ԭ����
template<typename T>
ConfigPair::ConfigPair(const std::string&name,const T&object):
	key(name),
	value(ConfigPair::get_config_string<T>(object)),
	type(GET_TYPE_NAME(T)),
	address((std::size_t)&object){}
template<typename Object>
std::string ConfigPair::get_config_string(const Object&field)
{
	std::ostringstream oss;
	if constexpr(std::is_fundamental<Object>::value&(!std::is_same<Object,char*>::value)) //��������,int,float,..,�����ų�char*
	{
		#ifdef __SERIALIZABLE_H__ //ֻ�����л���ʱ�����Ҫ���
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void //���ַ����л�ԭ����
		{
			std::istringstream iss(str);
			Object value;
			iss>>value;
			*(Object*)field=value;
		};
		#endif
		oss<<field;
		return oss.str();
	}
	else if constexpr(std::is_same<Object,std::string>::value)  //�ַ���,std::string
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void //ֻ���Թ�std::string
		{
			Object value;
			char ch;//�Ե�˫����
			std::istringstream iss(str);
			iss>>ch>>value;
			value.pop_back();
			*(Object*)field=value;
		};
		#endif
		oss<<"\""<<field<<"\"";
		return oss.str();
	}
	else if constexpr(std::is_same<Object,char*>::value)  //�ַ���,char*�����ûDEBGU�������п�
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			std::string value;
			char ch;//�Ե�˫����
			std::istringstream iss(str);
			iss>>ch>>value;
			value.pop_back();
			field=(void*)malloc(sizeof(char)*(value.size()+1)); //�����field�����nullptr
			memcpy(field,value.c_str(),value.size()+1);//��1����Ϊ'\0'
		};
		#endif
		oss<<"\""<<field<<"\"";
		return oss.str();
	}
	else if constexpr(std::is_pointer<Object>::value) //ָ��
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			using type=typename std::remove_pointer<Object>::type;
			Object value=nullptr;
			if(str!="null")
			{
				value=new typename std::remove_pointer<Object>::type();
				if constexpr(!IsSerializableType<Object>::value)
					from_config_string[GET_TYPE_NAME(type)](&(*value),str);
				else
					Object::from_config_string[GET_TYPE_NAME(type)](&(*value),str);
			}
			*(Object*)field=value;
		};
		#endif
		if(field==nullptr)
			return"null";
		return get_config_string<typename std::remove_pointer<Object>::type>(*field);
	}
	else if constexpr(IsSerializableType<Object>::value) //�����л��ķǻ�������
	{
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			Object::from_config_string[GET_TYPE_NAME(Object)](field,str);
		};
		#endif
		return field.get_config().serialized_to_string(false);
	}
	else if constexpr(IsTupleOrPair<Object>::value)
	{
		oss<<"[";
		for_each_element(field,[&](auto&it,int index){
			if(index+1<(int)std::tuple_size<Object>::value)
				oss<<get_config_string(it)<<",";
			else
				oss<<get_config_string(it);
		});
		oss<<"]";
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void 
		{
			auto values=unpacking_list(str);                             //�ַ�����ʽ��ֵ
			for_each_element(*(Object*)field,[&](auto&it,int index){
				std::string type_name=GET_TYPE_NAME(decltype(it));       //�õ�ÿ��Ԫ�ص�����
				from_config_string[type_name](&it,values[index]);	     //Ȼ���ҵ���Ӧ�ĺ���,���λ�ԭ
			}); //tupleÿ��λ��Ԫ�������ǲ���ı��
		};
		return oss.str();
	}
	else if constexpr(IsIterableType<Object>::value&(!std::is_same<Object,std::string>::value))//������
	{
		using element_type=typename std::remove_const<typename std::remove_reference<decltype(*field.begin())>::type>::type;
		std::ostringstream oss;
		oss<<"[";
		std::size_t index=0;
		for(auto&it:field)
		{
			index++;
			if(index==field.size()) //���һ��Ԫ��
				oss<<get_config_string<element_type>(it);
			else
				oss<<get_config_string<element_type>(it)<<",";
		}
		oss<<"]";
		#ifdef __SERIALIZABLE_H__
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			std::vector<std::string>values=unpacking_list(str);
			Object object(values.size());//Ԫ�ظ����ǿ���ȷ���ģ�һ���Է���ÿռ�
			int index=0;
			for(auto&it:object)                                     //Object������std::list,std::deque,���Բ���ֱ���±����
			{
				if constexpr(std::is_pointer<element_type>::value&&IsSerializableType<typename std::remove_pointer<element_type>::type>::value)
				{ //Serialiable����ָ��
					using remove_ptr_type=typename std::remove_pointer<element_type>::type;
					remove_ptr_type::from_config_string[GET_TYPE_NAME(element_type)](&it,values[index]);
				}
				else if constexpr(IsSerializableType<element_type>::value)
					element_type::from_config_string[GET_TYPE_NAME(element_type)](&it,values[index]); 
				else
					from_config_string[GET_TYPE_NAME(element_type)](&it,values[index]);//���Ԫ�ػ�ԭ
				++index;
			}	
			*(Object*)field=std::move(object);
		};
		#endif	
		return oss.str();
	}
	else if constexpr(IsArrayType<Object>::value) //C++ԭ������,int a[15];
	{
		using element_type=typename std::remove_const<typename std::remove_reference<decltype(field[0])>::type>::type;
		constexpr std::size_t length=sizeof(Object)/sizeof(element_type);
		oss<<"[";
		for(unsigned int i=0;i<length;i++)
		{
			oss<<get_config_string<element_type>(field[i]);
			if(i+1<length)
				oss<<",";
		}
		oss<<"]";
		from_config_string[GET_TYPE_NAME(Object)]=[](void*field,const std::string&str)->void
		{
			std::string type_name=GET_TYPE_NAME(element_type);
			auto values=unpacking_list(str);
			const std::size_t length=values.size();
			for(unsigned i=0;i<length;i++)
				from_config_string[type_name]((void*)((std::size_t)field+sizeof(element_type)*i),values[i]);
		};
		return oss.str();
	}
	return "<not serializable object>";
}
#endif
