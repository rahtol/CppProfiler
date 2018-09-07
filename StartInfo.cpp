#include "StartInfo.hpp"
//#include <fstream>
//#include <filesystem>
#include <Windows.h>

namespace
{
	//-------------------------------------------------------------------------
	bool dirExists(const std::wstring& dirName_in)
	{
		DWORD ftyp = GetFileAttributes(dirName_in.c_str());
		if (ftyp == INVALID_FILE_ATTRIBUTES)
			return false;  //something is wrong with your path!

		if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
			return true;   // this is a directory!

		return false;    // this is not a directory!
	}

	void CheckPathExists(const std::wstring& context, const std::wstring& path)
	{
//		std::ifstream f(path.c_str());
//		if (!std::filesystem::is_directory(path.c_str()))
		if (!dirExists(path))
			throw (context + L" \"" + path + L"\" does not exist");
	}

}

//-------------------------------------------------------------------------
StartInfo::StartInfo(const std::wstring& path)
	: path_(path)
{
	AddArgument(path);
}

//-------------------------------------------------------------------------
StartInfo::StartInfo(StartInfo&& startInfo)
	: path_{ std::move(startInfo.path_) }
	, arguments_( std::move(startInfo.arguments_) )
	, workingDirectory_{ std::move(startInfo.workingDirectory_) }
{
}

//-------------------------------------------------------------------------
void StartInfo::SetWorkingDirectory(const std::wstring& workingDirectory)
{
	CheckPathExists(L"Working directory", workingDirectory);
	workingDirectory_ = workingDirectory;					
}

//-------------------------------------------------------------------------
void StartInfo::AddArgument(const std::wstring& argument)
{
	arguments_.push_back(argument);
}

//-------------------------------------------------------------------------
const std::wstring& StartInfo::GetPath() const
{
	return path_;
}

//-------------------------------------------------------------------------
const std::vector<std::wstring>& StartInfo::GetArguments() const
{
	return arguments_;
}

//-------------------------------------------------------------------------
const MayBe<std::wstring> StartInfo::GetWorkingDirectory() const
{
	return workingDirectory_;
}

//-------------------------------------------------------------------------
std::wostream& operator<<(std::wostream& ostr, const StartInfo& startInfo)
{
	ostr << L"Path:" << startInfo.path_ << std::endl;
	ostr << L"Arguments:";

	const auto& arguments = startInfo.arguments_;

	for (size_t i = 1; i < arguments.size(); ++i)
		ostr << arguments[i] << " ";
	ostr << std::endl;
	ostr << L"Working directory: ";
	
	if (startInfo.workingDirectory_)
		ostr << startInfo.workingDirectory_;
	else
		ostr << L"not set.";
	return ostr;
}
