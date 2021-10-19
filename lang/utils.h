#ifndef __UNPACKING_H__
#define __UNPACKINT_H__
#include<vector>
#include<string>
#include<tuple>
#include<utility>
template<typename T>
struct IsSerializableType; //�����ڼ���Ƿ��г�Ա����get_config,���value=true��˵����������֧�����л��ķǻ�������(ʵ����get_config����) 
template<typename T>
struct IsIterableType;     //�Ƿ��ǿɵ����ģ�����std���� 
template<typename T>
struct IsTupleOrPair;
template<typename Object>
struct IsArrayType;	
template <typename T, size_t N>
inline constexpr size_t GetArrayLength(const T(&)[N]);
inline std::vector<std::string>unpacking_list(const std::string&serialized); //�б���,"[1,2,3,4]"->["1","2","3","4"]
template<typename Object,int index=0>
inline auto for_each_element(Object&object,auto&&callback);                 //����std::tuple,std::pairÿ��Ԫ��,���л�/�����л� ���뺯�����󼴿�

template<typename T>
struct IsSerializableType                                  //�����ڼ���Ƿ��г�Ա����get_config,���value=true
{                                                                      //˵����������֧�����л��ķǻ�������(ʵ����get_config����) 
    template<typename U>                                               //����ͨ��get_config���������л� 
        static auto check(int)->decltype(std::declval<U>().get_config(),std::true_type());
    template<typename U>
        static std::false_type check(...);
    static constexpr int value = std::is_same<decltype(check<T>(0)),std::true_type>::value;
};
template<typename T>
struct IsIterableType                                     //�Ƿ���е�����,��std::vector�����������л� 
{
    template<typename U>
        static auto check(int)->decltype(std::declval<typename U::iterator>(),std::true_type());
    template<typename U>
        static std::false_type check(...);
    static constexpr int value = std::is_same<decltype(check<T>(0)),std::true_type>::value;
};
template<typename Object>
struct IsTupleOrPair
{
	template<typename T>
	static constexpr auto check(int)->decltype(std::get<0>(std::declval<T>()),std::true_type());
	template<typename T>
	static constexpr auto check(...)->std::false_type;
	static constexpr int value=std::is_same<decltype(check<Object>(0)),std::true_type>::value;
};
template<typename Object>
struct IsArrayType
{
	template<typename T>
	static constexpr auto check(int)->decltype(GetArrayLength(std::declval<T>()),std::true_type());
	template<typename T>
	static constexpr auto check(...)->std::false_type;
	static constexpr int value=std::is_same<decltype(check<Object>(0)),std::true_type>::value;
};

template <typename T, size_t N>
inline constexpr size_t GetArrayLength(const T(&)[N])
{
    return N;
}
template<typename Object,int index=0>
inline auto for_each_element(Object&object,auto&&callback)                              //����ÿһ��Ԫ�ؽ��б���
{
	callback(std::get<index>(object),index);
	if constexpr(index+1<std::tuple_size<Object>::value)
		for_each_element<Object,index+1>(object,callback);
}
inline std::vector<std::string>unpacking_list(const std::string&serialized)              //�б���,"[1,2,3,4]"->["1","2","3","4"]
{                                                                                        //˼·�ο�Serializable::decode
	constexpr int init=0;
	constexpr int parse_fundamental=1;
	constexpr int parse_struct=2;
	constexpr int parse_iterable=3;
	constexpr int parse_string=4;
	constexpr int end_parse=5;
	int state=init;
	std::vector<std::string>vec;
	std::string temp;
	int length=serialized.size();
	int nested_struct=0;             //Ƕ�׵������{{},{}}
	int nested_iterable=0;           //Ƕ�׵������[[],[],{}]
	for(int i=0;i<length;++i)
	{
		auto&it=serialized[i];
		if(i==0)
			continue;
		if(state==init)
		{
			if(it=='{')
			{
				state=parse_struct;
				nested_struct++;
				temp.push_back(it);
			}
			else if(it=='[')
			{
				state=parse_iterable;
				nested_iterable++;
				temp.push_back(it);
			}	
			else if(it!=','&&it!=' ')
			{
				state=parse_fundamental;
				temp.push_back(it);
			}
			else if(it=='\"')
			{
				state=parse_string;
				temp.push_back(it);
			}
		}
		else if(state==parse_string)
		{
			temp.push_back(it);
			if(it=='\"'&&serialized[i-1]!='\\') //ת���ַ����ǽ���
			{
				state=end_parse;
				--i;
			}
		}
		else if(state==parse_struct)
		{
			if(it=='}'||it=='{')
				nested_struct+=(it=='}'?-1:1);
			if(nested_struct==0) //�������
			{
				state=end_parse;
				--i;
				temp.push_back(it);
				continue;
			}
			temp.push_back(it);
		}
		else if(state==parse_iterable)
		{
			if(it==']'||it=='[')
				nested_iterable+=(it==']'?-1:1);
			if(nested_iterable==0)
			{
				state=end_parse;
				--i;
				temp.push_back(it);
				continue;
			}
			temp.push_back(it);
		}
		else if(state==parse_fundamental)
		{
			if(it==','||it==']')
			{
				state=end_parse;
				--i;
				continue;
			}
			temp.push_back(it);
		}
		else if(state==end_parse)
		{
//			std::cout<<"<"<<temp<<">\n";
			vec.push_back(temp);
			temp.clear();
			state=init;
		}
	}
	return vec;
}
#endif
