/*
 * @Author: Zihan Zhang 
 * @Date: 2019-10-30 21:25:42 
 * @Last Modified by: Zihan Zhang
 * @Last Modified time: 2019-10-30 22:28:34
 */
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <thread>
#include <mutex>
#include <map>
#include <algorithm>

#include "Timer.h"
#include "notify.h"

#if defined(linux) || defined(__linux) || defined(__linux__)
//CPU core
#include <sys/sysinfo.h>
#else
#include <windows.h>
#include <Winbase.h>
#endif

inline int get_CPU_core_num()
{
#if defined(WIN32)
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#elif defined(LINUX) || defined(SOLARIS) || defined(AIX) || defined(linux) || defined(__linux__) || defined(__linux)
	// GNU fuction
	return get_nprocs();
#else
#error "not supported!"
#endif
}

using namespace std;
/*
OLD STANDARD:

   cargo:  “VesselType” = 70 OR “VesselType” = 71 OR “VesselType” = 72 OR “VesselType” = 73 OR “VesselType” = 74 OR “VesselType” = 75 OR “VesselType” = 76 OR “VesselType” = 77 OR “VesselType” = 78 OR “VesselType” = 79 
   tanker: “VesselType” = 80 OR “VesselType” = 81 OR “VesselType” = 82 OR “VesselType” = 83 OR “VesselType” = 84 OR “VesselType” = 85 OR “VesselType” = 86 OR “VesselType” = 87 OR “VesselType” = 88 OR “VesselType” = 89 
   tug and tow: “VesselType” = 31 OR “VesselType” = 32 OR “VesselType” = 52 
                and 21, 22
                
NEW-STANDARD SINCE 2018:
   cargo: 1003,1004,1016
   tanker: 1017,1024
   tug tow: 1023,1025
*/
const vector<unsigned int> SELECT_VESSEL_TYPE =  {1001, 1004};	// only for test file I currently have !!!!

	/* {
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 1017, 1024}; // only select tankers */

// if file with one line larger than this number, it is a illegal file
#define BUFFER_SIZE 10024
// vessel running state ...
#define VESSEL_RUN 1
#define VESSEL_STOP 0
// max legal time diffrence
#define MAX_TIME_DIFFRENCE 360

/*----------------------------------------------------------------------------------------------
APIs for string strip and split 
----------------------------------------------------------------------------------------------*/
std::string do_strip(const std::string &str,
					 int striptype,
					 const std::string &chars);

std::string strip(const std::string &str,
				  const std::string &chars = " ");

std::string lstrip(const std::string &str,
				   const std::string &chars = " ");

std::string rstrip(const std::string &str, 
				   const std::string &chars = " ");

// TODO: 学习一下使用正则表达式吧还是
void split(const string& s, vector<string>& token, const string& delimiter);


std::string do_strip(const std::string &str,
					 int striptype,
					 const std::string &chars)
{
	std::string::size_type strlen = str.size();
	std::string::size_type charslen = chars.size();
	std::string::size_type i, j;
	//默认情况下，去除空白符
	if (0 == charslen)
	{
		i = 0;
		//去掉左边空白字符
		if (striptype != 1)
		{
			// :: 作用域，表示选用全局的函数
			while (i < strlen && ::isspace(str[i]))
			{
				i++;
			}
		}
		j = strlen;
		//去掉右边空白字符
		if (striptype != 0)
		{
			j--;
			while (j >= i && ::isspace(str[j]))
			{
				j--;
			}
			j++;
		}
	}
	else
	{
		//把删除序列转为c字符串
		const char *sep = chars.c_str();
		i = 0;
		if (striptype != 1)
		{
			while (i < strlen && memchr(sep, str[i], charslen))
			{
				i++;
			}
		}
		j = strlen;
		if (striptype != 0)
		{
			j--;
			while (j >= i && memchr(sep, str[j], charslen))
			{
				j--;
			}
			j++;
		}
		if (0 == i && j == strlen)
		{
			return str;
		}
		else
		{
			return str.substr(i, j - i);
		}
	}
}

std::string strip(const std::string &str,
				  const std::string &chars)
{
	return do_strip(str, 2, chars);
}

std::string lstrip(const std::string &str,
				   const std::string &chars)
{
	return do_strip(str, 0, chars);
}

std::string rstrip(const std::string &str,
				   const std::string &chars)
{
	return do_strip(str, 1, chars);
}

void split(const string& s, vector<string>& token, const string& delimiter = ",")
{
	string::size_type las_pos = s.find_first_not_of(delimiter, 0);
	string::size_type pos = s.find_first_of(delimiter, las_pos);

	while(string::npos != pos || string::npos != las_pos)
	{
		token.push_back(s.substr(las_pos, pos-las_pos));
		las_pos = s.find_first_not_of(delimiter, pos);
		pos = s.find_first_of(delimiter,las_pos);
	}
}

inline time_t time2second(const string &time_str_in_excel)
{
	char *cha = (char *)time_str_in_excel.data();
	tm _tm;
	int year, month, day, hour, minute, second;
	sscanf(cha, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
	_tm.tm_year = year - 1900; // 年，由于tm结构体存储的是从1900年开始的时间，所以tm_year为int临时变量减去1900
	_tm.tm_mon = month - 1;	   // 月，由于tm结构体的月份存储范围为0-11，所以tm_mon为int临时变量减去1
	_tm.tm_mday = day;		   // 日
	_tm.tm_hour = hour;		   // 时
	_tm.tm_min = minute;	   // 分
	_tm.tm_sec = second;	   // 秒
	_tm.tm_isdst = 0;		   // 非夏令时
	time_t _t = mktime(&_tm);
	return _t;
}

inline string second2time(const time_t &time_in_second)
{
	tm *tm_ = localtime(&time_in_second);
	int year, month, day, hour, minute, second;
	year = tm_->tm_year + 1900; // 年，由于tm结构体存储的是从1900年开始的时间，所以临时变量int为tm_year加上1900
	month = tm_->tm_mon + 1;	// 月，由于tm结构体的月份存储范围为0-11，所以临时变量int为tm_mon加上1
	day = tm_->tm_mday;			// 日
	hour = tm_->tm_hour;		// 时
	minute = tm_->tm_min;		// 分
	second = tm_->tm_sec;		// 秒
	// 定义时间的各个char*变量
	char yearStr[5], monthStr[3], dayStr[3], hourStr[3], minuteStr[3], secondStr[3];
	sprintf(yearStr, "%d", year);	 
	sprintf(monthStr, "%d", month);   
	sprintf(dayStr, "%d", day);		  
	sprintf(hourStr, "%d", hour);	 
	sprintf(minuteStr, "%d", minute);
	sprintf(secondStr, "%d", second);
	// 如果分为一位，如5，则需要转换字符串为两位，如05
	if (minuteStr[1] == '\0')
	{
		minuteStr[2] = '\0';
		minuteStr[1] = minuteStr[0];
		minuteStr[0] = '0';
	}
	if (secondStr[1] == '\0')
	{
		secondStr[2] = '\0';
		secondStr[1] = secondStr[0];
		secondStr[0] = '0';
	}
	// 定义总日期时间变量
	char s[26];
	// 将年月日时分秒合并
	sprintf(s, "%s-%s-%sT%s:%s:%s", yearStr, monthStr, dayStr, hourStr, minuteStr, secondStr);
	string str_time(s);
	return str_time;
}

inline string status2string(const int vessel_state_int_value)
{
	if (VESSEL_RUN == vessel_state_int_value)
		return "running";
	else if (VESSEL_STOP == vessel_state_int_value)
		return "stop";
	else
		return "undefined";
}

// API for find the best L0 of longitude for coordinate transfering
int find_best_coordinate_transfer_L0(const float minlongitude, const float maxlongtitude)
{
	float midlongti = (minlongitude + maxlongtitude) / 2;
	int sign = 1;
	if (midlongti < 0)
		sign = -1;
	midlongti = abs(midlongti); 
	//-------------------------
	int L0_min = int(midlongti);
	int L0_max = L0_min;
	if ((L0_max - midlongti) < (midlongti - L0_min))
		return L0_max * sign;
	else
		return L0_min * sign;
}

// column name in original csv-style AIS data
enum FIELDS
{
	MMSI,
	BaseDateTime,
	LAT,
	LON,
	SOG,
	COG,
	Heading,
	VesselName,
	IMO,
	CallSign,
	VesselType,
	Status,
	Length,
	Width,
	Draft,
	Cargo
};

//--- hash table of vessel running status ---
// vessel status meaning
// if we failed to find the status string in the following table, we also treat it is in running state!
map<string, int> vessel_running_dictionary = {
	{"", VESSEL_RUN},
	{"undefined", VESSEL_RUN},
	{"under way using engine", VESSEL_RUN},
	{"under way sailing", VESSEL_RUN},
	{"AIS-SART (active); MOB-AIS; EPIRB-AIS", VESSEL_RUN},
	{"moored", VESSEL_STOP},
	{"at anchor", VESSEL_STOP},
	{"not under command", VESSEL_STOP},
	{"constrained by her draught", VESSEL_STOP},
	{"restricted maneuverability", VESSEL_STOP},
	{"reserved for future use (13)", VESSEL_STOP},
	{"power-driven vessel towing astern", VESSEL_STOP},
	{"power-driven vessel pushing ahead or towing alongside", VESSEL_STOP}};

struct POS
{
	
};

class MBR
{
public:
	MBR();
	MBR(const float *b) : boundary{b[min_longti], b[min_lanti],
								   b[max_longti], b[max_lanti]} {}
	~MBR();

public:
	void set(float *);
	void update(float, float);
	float *get_boundary();
	bool is_inside(float, float);

protected:
	enum
	{
		min_longti,
		min_lanti,
		max_longti,
		max_lanti
	};
	float boundary[4];
};

MBR::MBR() {}
MBR::~MBR() {}

void MBR::set(float b[])
{
	for (unsigned int i = 0; i < 4; ++i)
		boundary[i] = b[i];
}

void MBR::update(float longti, float lanti)
{
	if (longti < boundary[min_longti])
		boundary[min_longti] = longti;
	if (longti > boundary[max_longti])
		boundary[max_longti] = longti;
	if (lanti < boundary[min_lanti])
		boundary[min_lanti] = lanti;
	if (lanti > boundary[max_lanti])
		boundary[max_lanti] = lanti;
}

float *MBR::get_boundary()
{
	float *b = boundary;
	return b;
}

bool MBR::is_inside(float longti, float lanti)
{

	if (longti < boundary[max_longti] &&
		longti > boundary[min_lanti] &&
		lanti < boundary[max_longti] &&
		lanti > boundary[min_lanti])
		return true;
	else
		return false;
}

inline long estimate_record_num(istream &is)
{
	char buffer[BUFFER_SIZE];
	// estimate record numbers using 50 records
	is.seekg(0, ios::end);
	long long file_size = is.tellg();
	is.seekg(0, ios::beg);
	long long i = 0;
	for (i = 0; i < 50; ++i)
	{
		if (is.fail())
			break;
		is.getline(buffer, BUFFER_SIZE - 2);
	}
	if (i == 0)
	{
		cout << "[Error] bad file format" << endl;
		return 0;
	}
	int size1 = is.tellg();
	int iall = file_size / size1 * i;
	cout << "[OK] Total about " << iall << " records ..." << endl;
	// --- now estimate the mbr -------------------------
	is.clear();
	is.seekg(0, ios::beg);
	return iall;
}

/*
 each record is mapped to one line of excel file
 we let every member to be public, so that to accelerate the speed
*/
class VesselPos
{
	/*......................................................................................................................
     create a vessel_pos_record instance according to a line of AIS
     @parameters:
        - line_in_excel, line text extracted from an csv-formated AIS file if they are certain type of vessels specified in
                         SELECTED_VESSEL_TYPES
        - mbr_boudary,   only position in this boundary will be generated an instance
                         if mbr_boudary == None, will not limit positioning area!
     @return
        - if in the given area and is the specified vessel type, generate an instance
        - otherwise None is returned!
......................................................................................................................*/
	friend void create_instance(string&, VesselPos&, MBR&);
	static int get_running_state(const string&);
public:
	VesselPos();
	VesselPos(int, time_t, float, float, int, int);
	~VesselPos();
	string to_res();	
	friend class RecordTree;

public:
	int MMSI;
	time_t time_second;
	float longti;
	float lanti;
	int status;
	int time_diff = 0;
};

VesselPos::VesselPos() {}

VesselPos::~VesselPos() {}

VesselPos::VesselPos(int _MMSI,
					 time_t _time_second,
					 float _longti,
					 float _lanti,
					 int _status,
					 int _time_diff = 0)
{
	MMSI = _MMSI;
	time_second = _time_second;
	longti = _longti;
	lanti = _lanti;
	status = _status;
	time_diff = _time_diff;
}

int get_running_state(const string &s)
{
	map<string, int>::iterator iter;
	iter = vessel_running_dictionary.find(s);
	if (iter == vessel_running_dictionary.end())
		return VESSEL_RUN;
	else
		return iter->second;
}
// 原始数据转换
void create_instance(string &line_in_excel,
					 VesselPos& vp,
					 MBR    &mbr_boundary)
{
	string line = rstrip(line_in_excel);
	vector<string> data;
	split(line, data);
	
	if(data[VesselType] == "")
		return;
	
	int vessel_type = atoi(data[VesselType].c_str());
	int found = std::count(SELECT_VESSEL_TYPE.begin(), SELECT_VESSEL_TYPE.end(), vessel_type);

	if(found != 0)
	{
		int mmsi = atoi(data[MMSI].c_str());
		float latitude  = atof(data[LAT].c_str());
		float longitude = atof(data[LON].c_str());
		int run = get_running_state(data[Status]);
		if(mbr_boundary.is_inside(longitude, latitude))
		{
			vp.MMSI = mmsi;
			vp.time_second = time2second(data[BaseDateTime]);
			vp.longti = longitude;
			vp.lanti = latitude;
			vp.status = run;
			return;
		}
		else
			return;
	}
	else
		return;
	
	return;
}

string VesselPos::to_res()
{
	//char* ouput_line;
	string res;
	res = to_string(MMSI) + "," + to_string(longti) + "," + to_string(lanti) + "," + second2time(time_second) + "," + status2string(status) + "," + to_string(time_diff) + "\n";
	return res;
}

/*-------------------------------------------------------------------------
  we use hash map data structure to organize data

   (MMSI, vector)
   all position with the same MMSI are stored in the same vector
   and we will sort all those vectors using time information
   
   stop -> stop ,       ignore it
   stop -> runing,      insert it, interpolate = time difference / 2
   running -> running,  insert it, interpolate num = time difference
   running -> stop,     insert it, interpolate num = time difference / 2
-------------------------------------------------------------------------*/
class RecordTree
{
private:
	const int FSM_STATE_VESSEL_STOP = 0;
	const int FSM_STATE_VESSEL_RUNNING = 1;
public:
	RecordTree();
	//--- directly push a line of excel record to let it parse and create a record and push to the dictionary!
	bool push_line(string&, MBR&);
	// run -> run, we consider time difference = (pos_array[i].time_second - pos_array[i-1].time_second)
    // run -> stop, stop-> run, we consider time difference = (pos_array[i].time_second - pos_array[i-1].time_second)/2
    //                          because we do not know when 
	void filter_and_replace_positions();
	void interpolate_and_output(const char* ouput_file);
	// debug code to check whether is really sorted!!!    
	void dump(const char* fout);
	int datamap_size();
	// to interpolate points, do NOT call it outside the class!!!
	void _interpolate();
	// -- sort every vector, do NOT call it outside the class ------------
	void _sort();
	static bool compare_key(VesselPos&, VesselPos&);

public:
	int fsm;
	MBR mbr;
	map<int, vector<VesselPos>> data_map;
	int interpolated_points_num = 0;
	vector<int>* mmsi;
};

RecordTree::RecordTree():fsm(FSM_STATE_VESSEL_STOP) {}

bool RecordTree::push_line(string& excel_vessel_pos_record_line, MBR& mbr)
{
	VesselPos pos_record;
	create_instance(excel_vessel_pos_record_line, pos_record, mbr);
	if(pos_record.MMSI == 0)
		return false;
	if(data_map.count(pos_record.MMSI) == 0)
	{
		data_map.insert(pair<int, vector<VesselPos>>(pos_record.MMSI, vector<VesselPos>{}));
	}
	data_map[pos_record.MMSI].push_back(pos_record);
	return true;
}

void RecordTree::filter_and_replace_positions()
{
	cout << "[Debug] starting filter useless positioning data ..." << endl;
	map<int, vector<VesselPos>>::iterator iter = data_map.begin();
	for(; iter != data_map.end(); iter++)
	{
		vector<VesselPos> new_list;
		int map_size = data_map[iter->first].size();
		vector<VesselPos> pos_vector = iter->second;
		if(map_size > 0)
		{
			new_list.push_back(pos_vector[0]);
			fsm = pos_vector[0].status;
			// mbr.update(pos_vector[0].longti, pos_vector[0].lanti);
		}
		for (size_t i = 1; i < map_size; i++)
		{
			mbr.update(pos_vector[i].longti, pos_vector[i].lanti);
			// 上一个点静止
			if (fsm == FSM_STATE_VESSEL_STOP)
			{
				// 点静止
				if (pos_vector[i].status == FSM_STATE_VESSEL_STOP)
					// discard this point, two connected stop points
					continue;
				else
				// 点运动，keep it, need do nothing
				{
					pos_vector[i].time_diff = int((pos_vector[i].time_second - pos_vector[i - 1].time_second) / 2);
					new_list.push_back(pos_vector[i]);
					fsm = FSM_STATE_VESSEL_RUNNING;
					continue;
				}

			}
			// 上一个点运动
			else if(fsm == FSM_STATE_VESSEL_RUNNING)
			{
				// 点静止
				if (pos_vector[i].status == FSM_STATE_VESSEL_STOP)
				{
					fsm = FSM_STATE_VESSEL_STOP;
					pos_vector[i].time_diff = int((pos_vector[i].time_second - pos_vector[i - 1].time_second) / 2);
				}
				
				else
				// 点运动
				{
					pos_vector[i].time_diff = pos_vector[i].time_second - pos_vector[i - 1].time_second;
				}

				new_list.push_back(pos_vector[i]);
			}
			else
				cout << "[Error] Never should be here! Bad logic!" << endl;
		}
		pos_vector.clear();
		data_map[iter->first] = new_list;
	}
	cout << "[Debug] Finish filtering data" << endl;
}

void RecordTree::interpolate_and_output(const char* output_file)
{
	cout << "[Debug] starting to interpolate positioning data and save to file ..." << endl;
	interpolated_points_num = 0;
	ofstream os(output_file);
	if (!os.is_open())
	{
		cerr << "[Error] fail to open file and interpolate ..." << endl;
		return;
	}

	map<int, vector<VesselPos> >::iterator iter = data_map.begin();
	for(; iter != data_map.end(); iter++)
	{
		vector<VesselPos> pos_vector = iter->second;
		int iall = pos_vector.size();
		for (size_t i = 0; i < iall; i++)
		{
			if (iall > 0)
			{
				float dx, dy;
				dx = pos_vector[i].longti - pos_vector[i - 1].longti;
				dy = pos_vector[i].lanti - pos_vector[i - 1].lanti;
				int d_t = pos_vector[i].time_diff;
				
				if(d_t < MAX_TIME_DIFFRENCE && d_t >= 1)
				// interplote
				{
					for(size_t j = 1; j < d_t; j++)
					{
						float x = pos_vector[i-1].longti + dx * j / d_t;
						float y = pos_vector[i-1].lanti  + dy * j / d_t;
					
						os << to_string(x) << "," << to_string(y) << endl;
					}
					interpolated_points_num += d_t - 1;
				}
			}
		
			if (pos_vector[i].time_diff < 0)
			{
				cerr << "[Error] time_diff = " << interpolated_points_num << ", i=" << i <<  endl;
				return;
			}
			// whatever which i, always write it to file;
			os << to_string(pos_vector[i].longti) << "," << to_string(pos_vector[i].lanti) << endl;
		}
	}
	os.close();
	return;
}

bool RecordTree::compare_key(VesselPos& p1, VesselPos& p2)
{
	// 根据时间升序排列
	return p1.time_second < p2.time_second;
}

void RecordTree::_sort()
{
	cout << "Now sorting ..." << endl;
	int i = 0;
	map<int, vector<VesselPos>>::iterator iter = data_map.begin();
	
	for(; iter != data_map.end(); iter++)
	{
		vector<VesselPos> pos_vector = iter->second;
		sort(pos_vector.begin(), pos_vector.end(), compare_key); 
		iter->second = pos_vector;
		i += pos_vector.size();
	}

	cout << "[OK] All " << i << " lines" << " have been sorted."<< endl;
	return;
}

// debug code: write result to file
void RecordTree::dump(const char* fout)
{
	ofstream os(fout);
	if (!os.is_open())
	{
		cerr << "[Error] Bug when execute dump function ..." << endl;
		return;
	}
	map<int, vector<VesselPos>>::iterator iter = data_map.begin();
	
	for (; iter != data_map.end(); iter++)
	{
		vector<VesselPos> pos_vector = iter->second;
		vector<VesselPos>::iterator it = pos_vector.begin();
		
		for(; it!=pos_vector.end(); it++)
		{
			os << (*it).to_res();
		}
	}
	os.close();
	return;
}
// Debug code, just ignore ... 
int RecordTree::datamap_size()
{
	return data_map.size();
}

struct TaskInfo
{
    friend ostream& operator <<(ostream& os, const TaskInfo& info)
	{
		return (os << "ThreadID: " << info.id);
	}
public:
	std::thread::id      id;
	RecordTree*  	     rt; 
	vector<int>*         mmsi;
	int                  cursor;	// cursor of mmsi
};

class mutex_locker
{
public:
	mutex_locker() = delete;
	mutex_locker(std::mutex& locker) : _locker(locker)
	{
		_locker.lock();
	}
	~mutex_locker()
	{
		_locker.unlock();
	}
protected:
	std::mutex&  _locker;
};

//--------------------------------------------------------------------------------------------------------------------
//   class TaskCoordinator is dedicatedly designed for coordinate resource control during multi-thread interpolate
//   this class won't activate any thread, and it is just be called by threads stored in threadpool
//--------------------------------------------------------------------------------------------------------------------
class TaskCoordinator
{
public:
	//----------------------------------------------------------------------------------------------------------------
	// please be aware that, we will do some optimization on block size, so the parameter set could be changed during
	// optimization                 
	//----------------------------------------------------------------------------------------------------------------
	TaskCoordinator():__total_threads(4),__cursor(0) {}

	void init(RecordTree* rt, vector<int>* mmsi)
	{
		__rt = rt;
		__mmsi = mmsi;
		cout << "initial done!" << endl;
	}

	bool QueryTask(TaskInfo& info, const std::thread::id &thread_id)   //interface for thread, if no task, return false!
	{
		mutex_locker m_locker(__locker);
		// if we have processed all data, quit!
		int iall = __rt -> data_map.size();
		if (__cursor == iall)
		{
			__live_tasks.erase(thread_id);
			return false;
		}
		info.rt = __rt;
		info.mmsi = __mmsi;
		info.cursor = __cursor;
		__cursor++;
		__live_tasks[thread_id] = info;
		return true;
	}

	int get_total_threads() const
	{
		return __total_threads;
	}

protected:
	std::mutex            __locker;
	RecordTree*   		  __rt;
	vector<int>*		  __mmsi;
	int                   __cursor;
	int 		          __total_threads;
	std::map<std::thread::id, TaskInfo>  	      __live_tasks;
};

class ThreadPool
{
public:
	ThreadPool(TaskCoordinator& coor) : __coordinator(coor)
	{
#if (DEBUG_TREAD==1)
		int thread_num = 1;
#else
		int thread_num = coor.get_total_threads();
#endif    			
		__threads.resize(thread_num);
	}
	~ThreadPool()
	{
		for (auto it = __threads.begin(); it != __threads.end(); ++it)
		{
			delete (*it);
			*it = NULL;
		}
	}
	ThreadPool() = delete;

public:
	void start_all()
	{
		for (auto it = __threads.begin(); it != __threads.end(); it++)
		{

			(*it) = new std::thread(ThreadPool::thread_function, &__coordinator);
		}
		cout << "[Debug] created " << __threads.size() << " threads ..." << endl;
		return;
	}

 	void join_all_threads()
	{
		for (auto it = __threads.begin(); it != __threads.end(); it++)
		{
			// wait, let the thread which calling this function wait untill all those threads quit ...
			(*it) -> join();
		}
		return;
	}

protected:
	static void thread_function(TaskCoordinator *pcoordinator)
	{
		cout << "ThreadID [" << std::this_thread::get_id() << "] activated" << endl; 
		TaskCoordinator& coordinator = *pcoordinator;
		TaskInfo info;
		
		for (; coordinator.QueryTask(info, std::this_thread::get_id()) ;)
		{
			vector<VesselPos>& vp = (info.rt)-> data_map[(*info.mmsi)[info.cursor]];
			vector<VesselPos> pos_vector = (info.rt)-> data_map[(*info.mmsi)[info.cursor]];
			const int map_size = pos_vector.size();
			vector<VesselPos> new_list;
			int fsm = 0;
			if (map_size > 0)
			{
				new_list.push_back(pos_vector[0]);
				fsm = pos_vector[0].status;
				// mbr.update(pos_vector[0].longti, pos_vector[0].lanti);
			}
			for (size_t i = 1; i < map_size; i++)
			{
				// mbr.update(pos_vector[i].longti, pos_vector[i].lanti);
				// 上一个点静止
				if (fsm == 0)
				{
					// 点静止
					if (pos_vector[i].status == 0)
						// discard this point, two connected stop points
						continue;
					else
					// 点运动，keep it, need do nothing
					{
						pos_vector[i].time_diff = int((vp[i].time_second - pos_vector[i - 1].time_second) / 2);
						new_list.push_back(pos_vector[i]);
						fsm = 1;
						continue;
					}
				}
				// 上一个点运动
				else if (fsm == 1)
				{
					// 点静止
					if (pos_vector[i].status == 0)
					{
						fsm = 0;
						pos_vector[i].time_diff = int((pos_vector[i].time_second - pos_vector[i - 1].time_second) / 2);
					}

					else
					// 点运动
					{
						pos_vector[i].time_diff = pos_vector[i].time_second - pos_vector[i - 1].time_second;
					}
					new_list.push_back(pos_vector[i]);
				}
				else
					cout << "[Error] Never should be here! Bad logic!" << endl;
			}
			pos_vector.clear();
			vp = new_list;
		}

	}

protected:
	vector<std::thread*> __threads;
	TaskCoordinator&     __coordinator;
};

bool extract_data(const char *input_file,
				  const char *output_file,
				  MBR		 &mbr,
				  const char *debug_file1,
				  const char *debug_file2,
				  string     &output_desc_file
)
{	
	ifstream is(input_file);
	if (!is.is_open())
	{
		cerr << "[Error]: fail to open source file:" << input_file << endl;
		return false;
	}
	int iall_record = estimate_record_num(is);
	char buffer[BUFFER_SIZE];
	vector<std::string> ps;
	// 分配足够的空间
	ps.reserve(iall_record * 5);

	// 1. read data to memory
	cout << "Now read data to memory ..."       << endl;
	
	while (is.getline(buffer, BUFFER_SIZE - 2))
	{
		if (is.fail())
		{
			cerr << "[Error]: Error! When execute reading progress " << endl;
			return false;
		}
		ps.push_back(buffer);
	}
	cout << "[OK] All " << ps.size() << " have been launched" << endl;

	// 2. generate record using each lines' information
	RecordTree vessel_hash_table;
	cout << "Now create line instances ..."     << endl;

   	vector<string>::iterator iter_ps = ps.begin();
	for(; iter_ps != ps.end(); iter_ps++)
	{
		if (!vessel_hash_table.push_line(*iter_ps, mbr))
			continue;
	} 

	cout << "[OK] All " << vessel_hash_table.datamap_size() << " MMSIs types" << " have been read" << endl;

	// 3. sort and ouput data to debug file to check it
	vessel_hash_table._sort();
	vessel_hash_table.dump(debug_file1);

	vector<int> hash_table_mmsi;

	map<int, vector<VesselPos>>::iterator iter = vessel_hash_table.data_map.begin();
	for (; iter != (vessel_hash_table.data_map.end()); iter++)
		hash_table_mmsi.push_back(iter->first);

	// 4. filter out repeat stop point, and interpolate points
	// vessel_hash_table.filter_and_replace_positions();
   	TaskCoordinator coor;
	coor.init(&vessel_hash_table, &hash_table_mmsi);
	ThreadPool pool(coor);
	pool.start_all();
	pool.join_all_threads();

	vessel_hash_table.dump(debug_file2);
	vessel_hash_table.interpolate_and_output(output_file);
	// 5. write to description file
	cout << "Now write to description file ..." << endl;

	string str_boundary = "";
	string str_record = "";

	ofstream os(output_desc_file);
	if (!os.is_open())
	{
		cerr << "[Error]: fail to write file:" << output_file << endl;
		return false;
	}
	
	// TODO: str_boundary += ; 
	// TODO: str_record += ;

	// output additional information to description file
	os << "[Source file]: " << input_file << endl;
	os << "[Destination file]: " << output_file << endl;
	os << "[Extracted fields]: " << endl;
	os << str_record << endl;
	os << str_boundary << endl;

	// write a recommanded Lo for transferring Long/Lat to x/y to file!
	float* recommand_L0;
	recommand_L0 = mbr.get_boundary();

	int L0 = find_best_coordinate_transfer_L0(recommand_L0[0], recommand_L0[2]);
	// os << "L0 = " << to_string(L0) << endl;
	os << "The recommended L0  = " << to_string(L0) << endl;

	os.close();
	is.close();
	return true;
}

void help(const char **argv)
{
	cout << "========================================================================" << endl;
	cout << "Tools to extract tracks of tanker(AND ONLY tanker) to a new csv file" << endl;
	cout << "Usage:" << endl;
	cout << "  " << argv[0] << " source_AIS_file  [minx miny maxx maxy]" << endl;
	cout << "parameters:" << endl;
	cout << "       source_AIS_file - Source AIS file " << endl;
	cout << "       minx - minx of clip area " << endl;
	cout << "       miny - miny of clip area " << endl;
	cout << "       maxx - maxx of clip area " << endl;
	cout << "       maxy - maxy of clip area " << endl;
	cout << "If no minx,miny,maxx,maxy are specified, whole map will be processed" << endl;
	cout << "========================================================================" << endl;
}

int main(int argc, char const *argv[])
{
	// TODO: 增加可以选择全部区域的选项，现在竟然只能自己手动选
	if (argc != 6)
	{
		help(argv);
		return -1;
	}

	const char *input_longtilanti_file = argv[1];
	char *temp = new char[strlen(input_longtilanti_file) + 1];
	memcpy(temp, input_longtilanti_file, strlen(input_longtilanti_file) - 4);
	temp[strlen(input_longtilanti_file) - 4] = 0;
	string output_xy_file(temp);
	string output_xy_desc_file(temp);
	string output_xy_file_debug1(temp);
	string output_xy_file_debug2(temp);
	delete[] temp;

	// 设置一些输出文件的名称
	output_xy_file += "_long_lat.txt";
	output_xy_desc_file += + "_long_lat_MBR.txt";
	// TODO: 先不提供选项，两个debug文件都会生成
	// bool FILEOUT_DEBUG1 = true;
	// bool FILEOUT_DEBUG2 = true;
	output_xy_file_debug1 += "_long_lat_debug1.txt";
	output_xy_file_debug2 += "_long_lat_debug2.txt";

/* 	if (argc == 6)
	{ */
	float _mbr[4];
	// TODO: 将double转换为float时会出现精度缺失.
	_mbr[0] = atof(argv[2]);
	_mbr[1] = atof(argv[3]);
	_mbr[2] = atof(argv[4]);
	_mbr[3] = atof(argv[5]);

	MBR user_cut_mbr(_mbr);
	/* 	}  */

	cout << "Now open " << output_xy_file << " for reading data ... " << endl;

	if (!extract_data(input_longtilanti_file,
					  static_cast<const char *>(output_xy_file.c_str()),
					  user_cut_mbr,
					  output_xy_file_debug1.c_str(),
					  output_xy_file_debug2.c_str(),
					  output_xy_desc_file
					  ))
	{
		cerr << "[ERROR] Failed to extract data ..." << endl;
		return -1;
	}
	else
		cout << "[OK] Successfully tranferred to file " << output_xy_file << endl;

	return 0;
}
