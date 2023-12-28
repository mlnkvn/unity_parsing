#include <fstream>
#include <filesystem>
#include <iostream>
#include <utility>
#include <yaml-cpp/yaml.h>

using tag_t = long long;

struct SceneHierarchyParser {

private:
    std::map<tag_t, std::size_t> anchor_tags;
    std::vector<YAML::Node> scene_nodes;

public:

    int dump_scene_hierarchy(const std::string &scene_file_name, const std::string &output_folder_path) {
        map_tag_to_anchor(scene_file_name);
        scene_nodes = YAML::LoadAllFromFile(scene_file_name);

        auto pos = scene_file_name.find_last_of('/');
        std::string output_file = output_folder_path + '/' +
                                  scene_file_name.substr(pos + 1, scene_file_name.size() - pos - 1)
                                  + ".dump";
        std::ofstream f_out(output_file);

        for (const auto &node: scene_nodes) {
            const auto node_name = node.begin()->first.as<std::string>();
            if (node_name == "SceneRoots") {
                auto roots = node.begin()->second["m_Roots"];
                for (const auto &root: roots) {
                    auto tag = root.begin()->second.as<tag_t>();
                    dump_node_hierarchy(scene_nodes[anchor_tags[tag]], f_out);
                }
            }
        }

        f_out.close();
        return 0;
    }

private:
    int dump_node_hierarchy(const YAML::Node &node, std::ofstream &f_out, int depth = 0) {
        const auto &fields = node.begin()->second;
        const auto game_object_tag = fields["m_GameObject"].begin()->second.as<tag_t>();
        for (int i = 0; i < depth; ++i) {
            f_out << "--";
        }
        f_out << scene_nodes[anchor_tags[game_object_tag]].begin()->second["m_Name"].as<std::string>() << '\n';
        if (fields["m_Children"] && fields["m_Children"].IsSequence()) {
            for (const auto &child: fields["m_Children"]) {
                dump_node_hierarchy(scene_nodes[anchor_tags[child.begin()->second.as<tag_t>()]], f_out, depth + 1);
            }
        }
        return 0;
    }

    void map_tag_to_anchor(const std::string &scene_file_name) {
        std::ifstream scene_file(scene_file_name);
        if (!scene_file) {
            std::cerr << "Error while opening file " << scene_file_name << " occurred\n";
            exit(1);
        }
        std::string line;
        while (std::getline(scene_file, line)) {
            if (line.starts_with("---")) {
                line = line.substr(4, line.size() - 4);
                auto border = line.find('&');
                anchor_tags.emplace(std::stoll(line.substr(border + 1, line.size() - border - 1)), anchor_tags.size());
            }
        }
        scene_file.close();
    }
};

struct UnusedScriptsDetector {
private:
    std::string project_folder;
    std::map<std::string, std::string> all_scripts;

public:

    explicit UnusedScriptsDetector(const std::string &project_folder) : project_folder(project_folder) {
        for (const auto &entry: std::filesystem::recursive_directory_iterator(project_folder + "/Assets/Scripts")) {
            if (entry.path().extension() == ".cs") {
                std::string path = entry.path().string();
                std::ifstream meta_file(path + ".meta");
                path = path.substr(path.find_first_of('/') + 1);
                std::string line;
                std::string guid;
                while (std::getline(meta_file, line)) {
                    if (line.starts_with("guid")) {
                        std::size_t pos = 5;
                        while (pos < line.size() && std::iswspace(line[pos]) != 0) {
                            ++pos;
                        }
                        guid = line.substr(pos, line.size() - pos);
                        break;
                    }
                }
                all_scripts.emplace(guid, path);
            }
        }
    }

    void dump_unused_scripts(const std::string &output_folder_path) {
        std::ofstream f_out(output_folder_path + "/UnusedScripts.csv");
        std::string scenes_path = project_folder + "/Assets/Scenes";

        for (const auto &entry: std::filesystem::recursive_directory_iterator(scenes_path)) {
            if (entry.path().extension() == ".unity") {
                remove_used_scripts(entry.path().string());
            }
        }

        f_out << "Relative Path,GUID\n";
        for (const auto &[guid, path]: all_scripts) {
            f_out << path << ',' << guid << '\n';
        }
        f_out.close();
    }

private:
    void remove_used_scripts(const std::string &scene_path) {
        std::ifstream f_scene(scene_path);
        std::string line;
        while (std::getline(f_scene, line)) {
            if (auto pos = line.find("guid:"); pos != std::string::npos) {
                pos = pos + 5;
                while (pos < line.size() && std::iswspace(line[pos]) != 0) ++pos;
                std::size_t start = pos;
                std::size_t len = 0;
                while (pos < line.size() && std::isalnum(line[pos]) != 0) ++pos, ++len;
                auto guid = line.substr(start, len);
                if (auto it = all_scripts.find(guid); it != all_scripts.end()) {
                    all_scripts.erase(it);
                }
            }
        }
    }


};


int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Wrong number of arguments. Usage: ./tool.exe unity_project_path output_folder_path\n";
        return 1;
    }
    std::string unity_project_path = argv[1];
    std::string output_folder_path = argv[2];

    if (!std::filesystem::exists(unity_project_path)) {
        std::cerr << "Directory " << unity_project_path << " doesn't exist\n";
        return 1;
    }

    if (!std::filesystem::exists(output_folder_path)) {
        std::cout << "Directory " << unity_project_path << " doesn't exist\n" <<
                  "Create new directory " << unity_project_path << "? [y/n]\n";
        while (true) {
            std::string answer;
            std::cin >> answer;
            if (answer.starts_with('y') || answer.starts_with('Y')) {
                if (std::filesystem::create_directory(output_folder_path)) {
                    std::cout << "Directory " << output_folder_path << " was created\n";
                    break;
                } else {
                    std::cerr << "Failed to create directory: " << output_folder_path << '\n';
                    return 1;
                }
            } else if (answer.starts_with('n') || answer.starts_with('N')) {
                std::cerr << "Directory " << unity_project_path << " doesn't exist\n";
                return 1;
            } else {
                std::cout << "Create new directory " << unity_project_path << "?\n" <<
                          "Type 'y' or 'n' to continue.\n";
            }
        }
    }

    UnusedScriptsDetector unused_detector(unity_project_path);
    unused_detector.dump_unused_scripts(output_folder_path);
    std::string scenes_path = unity_project_path + "/Assets/Scenes";

    for (const auto &entry: std::filesystem::recursive_directory_iterator(scenes_path)) {
        if (entry.path().extension() == ".unity") {
            SceneHierarchyParser parser;
            parser.dump_scene_hierarchy(entry.path().string(), output_folder_path);
        }
    }


}
