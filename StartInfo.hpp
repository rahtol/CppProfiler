#pragma once

#include <string>
#include <vector>
#include <iosfwd>
#include "MayBe.h"

class StartInfo	{
	public:
		explicit StartInfo(const std::wstring&);
		
		StartInfo(StartInfo&&);

		StartInfo(const StartInfo&) = default;		
		StartInfo& operator=(const StartInfo&) = default;

		void SetWorkingDirectory(const std::wstring&);
		void AddArgument(const std::wstring&);

		const std::wstring& GetPath() const;
		const std::vector<std::wstring>& GetArguments() const;
		const MayBe<std::wstring> GetWorkingDirectory() const;

		friend std::wostream& operator<<(std::wostream& ostr, const StartInfo&);

	private:
		std::wstring path_;
		std::vector<std::wstring> arguments_;
		MayBe<std::wstring> workingDirectory_;
};
