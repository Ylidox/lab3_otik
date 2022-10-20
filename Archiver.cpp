#include <filesystem>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <bitset>
#include <map>

using namespace std;
namespace fs = std::filesystem;

class Group {
private:
    string code = "";
    double frequency = 0;
    unsigned char symbol = 0;
    vector<Group> list;
public:
    string get_code(){ return code; };
    double get_frequency(){ return frequency; };
    unsigned char get_symbol(){ return symbol; };
    vector<Group>& get_list(){ return list; };

    void set_code(string c) { code = c; };
    void set_frequency(double f) { frequency = f; };
    void set_symbol(unsigned char s) { symbol = s; };

    void pass_code(string code) {
        this->code = code;
        if (list.size() == 2) {
            list[0].pass_code(code + "0");
            list[1].pass_code(code + "1");
        }
    }

    void get_alphabet(map<string, unsigned char>& alphabet) {
        if (symbol != 0) {
            alphabet[code] = symbol;
        }
        else if (list.size() == 2) {
            list[0].get_alphabet(alphabet);
            list[1].get_alphabet(alphabet);
        }
    }

    Group(){}
    Group(unsigned char symb, double freq) : symbol(symb), frequency(freq) {}

    ~Group() {
        list.clear();
    }
};

class TextCompressor {
private:
    vector<Group> list;
    map<unsigned char, int> symbols;
    map<string, unsigned char> decoding_alphabet;
    map<unsigned char, string> encoding_alphabet;
    string encode_message;

    void sort() {
        for (size_t i = 0; i < list.size(); i++) {
            for (size_t j = 0; j < list.size() - 1; j++) {
                if (list[j].get_frequency() <= list[j + 1].get_frequency()) {
                    Group b = list[j];
                    list[j] = list[j + 1];
                    list[j + 1] = b;
                }
            }
        }
    }

    void set_group(int length) {
        map <unsigned char, int>::iterator it = symbols.begin();

        for (int i = 0; it != symbols.end(); it++, i++) {
            list.push_back(Group(it->first,(double) it->second / length));
        }
    }

    void build_tree() {
        while (list.size() != 1) {
            int last = (int)list.size() - 1;
            Group g;
            g.set_frequency(list[last].get_frequency() + list[last - 1].get_frequency());
            g.get_list().push_back(list[last]);
            g.get_list().push_back(list[last - 1]);
            list.pop_back();
            list.pop_back();
            list.push_back(g);

            sort();
        }
    }

    void generate_keys() {
        list[0].pass_code("");
    }
    
    void set_alphabet() {
        list[0].get_alphabet(this->decoding_alphabet);
        map<string, unsigned char>::iterator it;
        for (it = decoding_alphabet.begin(); it != decoding_alphabet.end(); it++)
        {
            encoding_alphabet[it->second] = it->first;
        }
    }

    int get_length_encode_message() {
        int out = 0;
        map<unsigned char, int>::iterator it;
        for (it = symbols.begin(); it != symbols.end(); it++)
        {
            out += it->second * encoding_alphabet[it->first].size();
        }
        return out;
    }

    string get_string_from_bites(string bit) {
        string out = "";
        bitset<8> bs;
        int count = 0;
        for (size_t i = 0; i < bit.size(); i++) {
            bs.set(count++, (bit[i] - '0') & 1);
            if (count == 8) {
                out += (char)bs.to_ulong();
                count = 0;
                bs.reset();
            }
        }
        if (count != 0) out += (char)bs.to_ulong();
        return out;
    }
    
    string get_bites_from_string(string message) {
        string out = "";
        for (size_t i = 0; i < message.size(); i++) {
            if (message[i] == '\r') continue;

            bitset<8> bs{ (unsigned char)message[i] };
            out += bs.to_string();
            bs.reset();
            
        }
        return out;
    }

    string build_encode_message(string message) {
        string bit = "";
        for (size_t i = 0; i < message.size(); i++) {
            bit += encoding_alphabet[(unsigned char)message[i]];
        }
        
        return get_string_from_bites(bit);
    }

    void set_decoding_alphabet_from_string(string str) {
        vector<unsigned char> keys;
        vector<string> values;
        string cur;
        for (auto& symb : str) {
            if (symb == '|') {
                values.push_back(cur);
                cur = "";
            }
            else if (symb == ':') {
                keys.push_back(stoi(cur));
                cur = "";
            }
            else {
                cur += symb;
            }
        }

        for (size_t i = 0; i < keys.size(); i++) {
            decoding_alphabet[values[i]] = keys[i];
        }

        keys.clear();
        values.clear();
    }

    void encrypt_message(string message) {
        for (size_t i = 0; i < message.size(); i++) {
            if (symbols.count(message[i])) symbols[message[i]]++;
            else {
                symbols[message[i]] = 1;
            }
        }

        this->set_group(message.size());

        this->sort();

        this->build_tree();

        this->generate_keys();

        this->set_alphabet();
    }

    string reverse(string const& s)
    {
        string rev(s.rbegin(), s.rend());
        return rev;
    }
public:
    TextCompressor(){}

    string get_string_from_encoding_alphabet() {
        string out = "";
        map<unsigned char, string>::iterator it;
        for (it = encoding_alphabet.begin(); it != encoding_alphabet.end(); it++)
        {
            out += to_string(it->first);
            out += ':' + it->second + "|";
        }
        return out;
    }
    string encode(string message) {
        this->encrypt_message(message);
        encode_message = build_encode_message(message);
        return encode_message;
    }

    string encode(vector<string>& vec_message, vector<int>& file_sizes) {
        string message = "";
        for (auto& word : vec_message) {
            message += word;
        }
        this->encrypt_message(message);

        string out = "";
        for (auto& word : vec_message) {
            string encode_word = build_encode_message(word);
            file_sizes.push_back(encode_word.size());

            out += encode_word;
        }
        return out;
    }

    string gecode(string alphabet, string message) {
        set_decoding_alphabet_from_string(alphabet);

        string bit = "";
        for (auto& symb : message) {
            bitset<8> bs{ (unsigned char)symb };
            bit += reverse(bs.to_string());
            bs.reset();
        }

        string out = "";
        string word = "";
        for (size_t i = 0; i < bit.size(); i++) {
            word += bit[i];
            if (decoding_alphabet.count(word)) {
                out += decoding_alphabet[word];
                word = "";
            }
        }
        out;
        return out;
    }
};

class File {
public:
    string name;
    int code = 0;
    string compressed_data;
    string data;
    string alphabet;
    File(){}
    File(string name, int code, string alph) : name(name), code(code), alphabet(alph) {}
};

class Coder {
private:
    string root;
    string signature = "JasonStatham";
    string version = "v3.0";
    string code_algorithm = "";
    string extension = ".jast";
    string mode = "separate";
    string target;
    vector<unsigned char> bytes;

    string buffer;
    vector<File> files;
public:
    Coder() {}
    vector<unsigned char>& get_bytes() { return bytes; };
    string get_root() {
        return this->root;
    }
    string get_signature() {
        return this->signature;
    }
    string get_version() {
        return this->version;
    }
    string get_code_algorithm() {
        return this->code_algorithm;
    }
    string get_mode() {
        return this->mode;
    }
    string get_target() {
        return this->target;
    }
    string get_extension() {
        return this->extension;
    }
    string get_buffer() {
        return this->buffer;
    }
    vector<File>& get_files() { return files; }

    void set_root(string root) {
        this->root = root;
    }
    void set_signature(string signature) {
        this->signature = signature;
    }
    void set_version(string version) {
        this->version = version;
    }
    void set_code_algorithm(string code_algorithm) {
        this->code_algorithm = code_algorithm;
    }
    void set_mode(string mode) {
        this->mode = mode;
    }
    void set_buffer(string vec) {
        buffer = vec;
    }
    void set_target(string target) {
        this->target = target;
    }
    void set_extension(string extension) {
        this->extension = extension;
    }
    void insert(vector<unsigned char>& vec) {
        buffer.insert(buffer.end(), vec.begin(), vec.end());
    }

    bool chek_file(string fileName) {
        ifstream file(fileName);
        if (!file) return false;
        file.close();
        return true;
    }
    string read_compressed_file(string fileName) {

        std::ifstream input(fileName, std::ios::binary);

        // copies all data into buffer
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
        string out = "";

        for (auto& letter : buffer) {
            out += letter;
        }

        return out;
    }

    string read(string fileName) {
        if (!chek_file(fileName)) cout << "FileName Error!" << endl;
        string buff;
        ifstream file(fileName);
        //if (!file) return 1;

        char b;
        while (file.get(b)) {
            buff += (char)b;
            bytes.push_back(b);
        }
        file.close();

        return buff;
    }

    string hide_extension(string filename) {
        string out = "";
        int len = 0;
        int i;
        for (i = filename.size() - 1; i >= 0; i--) {
            if (filename[i] == '.') break;
        }
        len = i;
        for (int i = 0; i < len; i++) {
            out += filename[i];
        }

        return out;
    }

    ~Coder() {
        buffer.clear();
        files.clear();
    }
};

class Encoder : public Coder {
private:    
    void print(vector<unsigned char>& arr) {
        for (size_t i = 0; i < arr.size(); i++) {
            cout << (char)arr[i];
        }
        cout << endl;
    }

    void printFiles() {
        for (size_t i = 0; i < this->get_files().size(); i++) {
            cout << this->get_files()[i].name << endl;
            cout << this->get_files()[i].data << endl;
        }
    }

    void readFolder() {
        string path = this->get_root() + "/" + this->get_target(); // "C:/Users/User/source/repos/coder_pro/coder_pro/test";
        for (const auto& entry : fs::directory_iterator(path)) {
            File f;

            f.name = entry.path().filename().u8string();
            f.data = read(get_target() + "/" + f.name);
            
            this->get_files().push_back(f);
        }
    }

    void readFile() {
        File f;
        f.data = read(get_target());
        this->get_files().push_back(f);
    }

    int size(string str) {
        int out = 0;
        for (auto& l : str) {
            if (l != '\r') out++;
        }
        return out;
    }

    void build() {
        string information = this->get_signature() + " "
            + this->get_version() + " " 
            + to_string(this->get_files().size()) + " "
            + this->get_mode() + "\n";
        string data = "";
        
        if (get_mode() == "separate") {
            for (size_t i = 0; i < this->get_files().size(); i++) {
                TextCompressor tc;
                string compressed_data = tc.encode(get_files()[i].data);
                string alphabet = tc.get_string_from_encoding_alphabet();

                cout << "File " << get_files()[i].name << endl;
                cout << "len comp_d: " << compressed_data.size() << endl;
                cout << "len alph: " << alphabet.size() << endl;
                cout << "len data: " << get_files()[i].data.size() << endl << endl;

                if (compressed_data.size() + alphabet.size() < get_files()[i].data.size()) {
                    data += compressed_data;

                    information += get_files()[i].name + " ";
                    information += "1 " + to_string(compressed_data.size()) + " ";
                    information += alphabet + "\n";
                }
                else {

                    data += get_files()[i].data;

                    information += get_files()[i].name + " ";
                    information += "0 " + to_string( get_files()[i].data.size()) + " ";
                    information += ".\n";
                }
            }
        }
        else if (get_mode() == "common") {
            vector<string> file_data;
            vector<int> count_bytes_compressed_files;

            string clean_data = "";

            for (size_t i = 0; i < this->get_files().size(); i++) {
                file_data.push_back(get_files()[i].data);
                clean_data += get_files()[i].data;
            }

            TextCompressor tc;
            string compressed_data = tc.encode(file_data, count_bytes_compressed_files);
            string alphabet = tc.get_string_from_encoding_alphabet();

            cout << "len comp_d: " << compressed_data.size() << endl;
            cout << "len alph: " << alphabet.size() << endl;
            cout << "len data: " << clean_data.size() << endl;

            if (compressed_data.size() + alphabet.size() < clean_data.size()) {
                information += get_files()[0].name + " " + "1 "
                    + to_string(count_bytes_compressed_files[0]) + " "
                    + tc.get_string_from_encoding_alphabet() + "\n";
                for (size_t i = 1; i < get_files().size(); i++) {
                    information += get_files()[i].name + " " + "1 "
                        + to_string(count_bytes_compressed_files[i]) + " .\n";
                }

                data = compressed_data;
            }
            else {
                information += get_files()[0].name + " " + "0 "
                    + to_string(get_files()[0].data.size()) + " .\n";
                for (size_t i = 0; i < get_files().size(); i++) {
                    information += get_files()[i].name + " " + "0 "
                        + to_string(get_files()[i].data.size()) + " .\n";
                }

                data = clean_data;
            }

            file_data.clear();
            count_bytes_compressed_files.clear();
        }

        set_buffer(information + data);
    }

public:
    Encoder() {}
    Encoder(string root, string target) {
        this->set_root(root);
        this->set_target(target);
    }
    Encoder(string root, string target, string mode) {
        this->set_root(root);
        this->set_target(target);
        this->set_mode(mode);
    }
    void encode() {
        this->readFolder();

        this->build();

        ofstream fout(get_target() + this->get_extension());
        fout << get_buffer();
        fout.close();
    }
};

class Decoder : public Coder {
private:
    int count_files = 0;
    

    bool check_signature(string signature) {
        return signature == this->get_signature();
    }
    bool check_version(string version) {
        return version == this->get_version();
    }
    bool check_algorithm(string code) {
        return (code[0] - '0') & 1 & (code.size() == 1);
    }

    vector<string> split(char separator, string end) {
        vector<string> out;
        string buf = get_buffer();
        
        string sub_str = "";
        for (size_t i = 0; i < buf.size() - end.size(); i++) {
            //if(buf[i] == end[0] && buf[i + 1] == end[1] && buf[i + 2] == end[2])
            bool flag = true;
            for (size_t j = 0; j < end.size(); j++) {
                if (buf[i + j] != end[j]) flag = false;
            }
            if (flag) {
                sub_str = buf.substr(0, i);
                buf.erase(0, i + end.size());

                set_buffer(buf);
                break;
            }
        }

        string word = "";
        for (auto& letter : sub_str) {
            if (letter == separator) {
                out.push_back(word);
                word = "";
            }
            else {
                word += letter;
            }
        }
        out.push_back(word);

        return out;
    }
    bool check_header() {
        vector<string> header = split(' ', "\r\n");
        if(!check_signature(header[0])) return false;
        if(!check_version(header[1])) return false;
        
        count_files = atoi(header[2].c_str());

        set_mode(header[3]);
        
        header.clear();
        return true;
    }

    void read_files() {
        vector<int> count_bytes;
        for (int i = 0; i < count_files; i++) {

            vector<string> file = split(' ', "\r\n");

            get_files().push_back(File(file[0], atoi(file[1].c_str()), file[3]));
            
            count_bytes.push_back(atoi(file[2].c_str()));
            file.clear();
        }

        
        int count = 0;
       
        string buf = get_buffer();

        size_t file = 0;
        string data = "";

        int len = 0;
        for (size_t j = 0; j < buf.size(); j++) {
            if (buf[j] == '\r' && get_files()[file].code == 1){
                len++;
                data += buf[j];
            }
            else if (buf[j] != '\r' && get_files()[file].code == 0) {
                len++;
                data += buf[j];
            }
            else if(buf[j] != '\r') {
                len++;
                data += buf[j];
            }

            if (len == count_bytes[file]) {
                (get_files()[file].code == 0) ?
                    get_files()[file].data = data :
                    get_files()[file].compressed_data = data;
                data = "";
                len = 0;
                file++;
            }
        }

        
        data = "";

        count_bytes.clear();
    }

    void create_folder(string name_folder) {
        fs::create_directory(this->get_root() + "/" + name_folder);
    }
    void create_file(string file_name, string content) {
        ofstream fout(file_name);
        fout << content;
        fout.close();
    }
    
    void decode_files() {
        string folder = this->hide_extension(this->get_target());
        create_folder(folder);

        string common_alphabet = get_files()[0].alphabet;
        for (size_t i = 0; i < get_files().size(); i++) {
            TextCompressor tc;
            string message;
            if (get_mode() == "separate") {
                message = (get_files()[i].code) ?
                    tc.gecode(get_files()[i].alphabet, get_files()[i].compressed_data) :
                    get_files()[i].data;
            }
            else if(get_mode() == "common") {
                message = (get_files()[i].code) ?
                    tc.gecode(common_alphabet, get_files()[i].compressed_data) :
                    get_files()[i].data;
            }
            
            create_file(folder + '/' + get_files()[i].name, message);
            
        }
        
    }
public:
    Decoder() {}
    Decoder(string root, string target) {
        this->set_root(root);
        this->set_target(target);
    }
    Decoder(string root, string target, string mode) {
        this->set_root(root);
        this->set_target(target);
        this->set_mode(mode);
    }
    void decode() {
        set_buffer(this->read_compressed_file(this->get_target()));
        
        if(!check_header()) return;
        
        read_files();

        decode_files();
       
    }
};

int main(int argc, char** argv)
{
    // encode test separate
    // encode test common
    // decode test.jast

    string action;
    string target;
    string mode;

    if (argc == 1) {
        cout << "Error! No arguments!" << endl;
    }
    else {

        action = string(argv[1]);
        target = string(argv[2]);
        if (action == "encode") {
            mode = string(argv[3]);
        }
    }

    if (action == "encode") {
        Encoder A("C:/Users/User/source/repos/Archiver/Archiver", target, mode);
        A.encode();
    }
    else if (action == "decode") {
        Decoder B("C:/Users/User/source/repos/Archiver/Archiver", target);
        B.decode();
    }


}