all: maldives

maldives: main.cpp memload.c get_pgd.c call_graph.cpp common.h global_data_layout.cpp rel_entry.cpp no_patch.cpp rel_data.cpp rel_rodata.cpp
	g++ -std=c++0x main.cpp memload.c get_pgd.c call_graph.cpp global_data_layout.cpp rel_entry.cpp no_patch.cpp rel_data.cpp rel_rodata.cpp -o maldives -g

clean:
	rm -rf maldives
