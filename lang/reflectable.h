#ifndef __REFLECTABLE_H__
#define __REFLECTABLE_H__
#include"config.h"
#include<vector>
#include<functional>
struct Reflectable
{
public:
	virtual ~Reflectable(){};
	template<typename T,typename ...Args>
	struct Regist;
	template<typename T>
	struct Regist<T>;
	template<typename T>
	Config get_config(const T*object)const;//�õ�T��������Ϣ������,�ж�T���͵����Լ�ֵ���Ƿ���,û�н����ͽ���(ֻ����һ��) 
	template<typename ClassType,typename FieldType>
	static void set_field(ClassType&object,std::string field_name,const FieldType&data);//��������ֵ����Ϊ�Ѿ���������Ϣ�����Բ���Ҫ����default_constructors[type]����ĺ��������� 
	template<typename FieldType=void*,typename ClassType>
	static FieldType get_field(ClassType&object,std::string field_name); 
	template<typename FieldType=void*>
	static FieldType get_field(void*object,std::string class_name,std::string field_name); 
	static std::string&get_field_type(std::string class_name,std::string field_name); 
	static void*get_instance(std::string class_name);
	static void delete_instance(std::string class_name,void*object); 
	using ClassName=std::string;
	using FieldName=std::unordered_map<std::string,std::pair<std::string,std::size_t>>;
	static std::unordered_map<ClassName,FieldName>field;
//private:	
	static std::unordered_map<ClassName,std::function<void*(void)>>default_constructors;
	static std::unordered_map<ClassName,std::function<void(void*)>>default_deconstructors;
};
std::unordered_map<std::string,std::function<void*(void)>>Reflectable::default_constructors;
std::unordered_map<std::string,std::function<void(void*)>>Reflectable::default_deconstructors;
std::unordered_map<std::string,std::unordered_map<std::string,std::pair<std::string,std::size_t>>>Reflectable::field;
template<typename T>
Config Reflectable::get_config(const T*object)const
{
	std::string class_name=GET_TYPE_NAME(T);
	Config config(
		field.find(class_name)==field.end()?&field[class_name]:nullptr,object);//��������ھʹ��������������� 
	config.update({{"class_name",class_name}});
	return config;
}
template<typename ClassType,typename FieldType>
void Reflectable::set_field(ClassType&object,std::string field_name,const FieldType&data)
{
	std::string class_name=GET_TYPE_NAME(ClassType);
	std::size_t offset=Reflectable::field[class_name][field_name].second;
	*(FieldType*)((std::size_t)(&object)+offset)=data;
}
template<typename FieldType=void*,typename ClassType>
FieldType Reflectable::get_field(ClassType&object,std::string field_name)
{
	std::string class_name=GET_TYPE_NAME(ClassType);
	std::size_t offset=Reflectable::field[class_name][field_name].second;
	if constexpr(std::is_same<FieldType,void*>::value)
		return (void*)((std::size_t)(&object)+offset);
	else
		return std::ref(*(FieldType*)((std::size_t)(&object)+offset));
}
template<typename FieldType=void*>
FieldType Reflectable::get_field(void*object,std::string class_name,std::string field_name)
{
	std::size_t offset=Reflectable::field[class_name][field_name].second;
	if constexpr(std::is_same<FieldType,void*>::value)
		return (void*)((std::size_t)(object)+offset);
	else
		return std::ref(*(FieldType*)((std::size_t)(&object)+offset));
}
std::string&Reflectable::get_field_type(std::string class_name,std::string field_name)
{
	return Reflectable::field[class_name][field_name].first;
}
void*Reflectable::get_instance(std::string class_name)
{
	return Reflectable::default_constructors[class_name]();
}
void Reflectable::delete_instance(std::string class_name,void*object)
{
	default_deconstructors[class_name](object);
}
template<typename T,typename ...Args>
struct Reflectable::Regist
{
	Regist()
	{
		T object;
		object.get_config();//�������get_config,���ܽ���������Ϣ,������������ȵ���һ��Reflectable��get_config 
		Reflectable::default_constructors[GET_TYPE_NAME(T)]=[](void)->void* //Ĭ�Ϲ��캯�� 
		{
			return (void*)(new T());
		};
		Reflectable::default_deconstructors[GET_TYPE_NAME(T)]=[](void*object)->void //�������� 
		{
			delete ((T*)object);
		};
		Regist<Args...>();
	}
};
template<typename T>
struct Reflectable::Regist<T>
{
	Regist()
	{
		T object;
		object.get_config();
		Reflectable::default_constructors[GET_TYPE_NAME(T)]=[](void)->void* //Ĭ�Ϲ��캯�� 
		{
			return (void*)(new T());
		};
		Reflectable::default_deconstructors[GET_TYPE_NAME(T)]=[](void*object)->void //�������� 
		{
			delete ((T*)object);
		};
	}
};
#endif
