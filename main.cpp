#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

bool Preprocess_recurs(const path& in_file, ofstream& out, const path& current_file, const vector<path>& include_directories, int line_number)
{
    
    ifstream in(in_file);
    if(!in)
        return false;
    

    static regex file_include(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex biblio_include(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    smatch m;
    string line;
    
    path parent = in_file.parent_path();
    while(getline(in, line))
    {
        line_number++;
        if(regex_match(line, m, file_include))
        {
            path p = string(m[1]);
            string ps = p.string();
            bool found = false;
            if(filesystem::exists(parent/p))
            {
                Preprocess_recurs(parent/p, out, current_file.string(), include_directories, line_number);
            }
            else{
                for (const auto& dir : include_directories) {
                    //line_number++;
                    path is_path = dir/p;
                    if(filesystem::exists(is_path))
                    {
                        Preprocess_recurs(dir/p, out, current_file.string(), include_directories, line_number);
                        found = true;
                    }
                   
                }
                if (!found) {
                    cout << "unknown include file " << m[1].str()
                    << " at file " << current_file.string()
                    << " at line " << line_number << endl;
                    return false;
                }
            }
            
            
        }
        else if (regex_match(line, m, biblio_include)) {
            path p =string(m[1]);
            path parent2 = p.parent_path();
            bool found = false;
            for (const auto& dir : include_directories) {
                //line_number++;
                path is_path = dir/p;
                if(filesystem::exists(is_path))
                {
                    Preprocess_recurs(dir/p, out, current_file.string(), include_directories, line_number);
                    found = true;
                }
               
            }
            if (!found) {
                cout << "unknown include file " << m[1].str()
                << " at file " << current_file.string()
                << " at line " << line_number << endl;
                return false;
            }
        } else {
            out << line << endl;
           
            
        }
    }
    return true;
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories)
{
    ifstream in(in_file);
    if(!in)
        return false;
    ofstream out(out_file);
    
    int line_number = 0;
    if(!Preprocess_recurs(in_file, out, in_file, include_directories, line_number))
    {
        return false;
    }
    
    
    return true;
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
