#include<cstdio>
#include<vector>
using namespace std;

class Node {
	int n; //количество ключей
	vector<int> keys;
	vector<Node*> children;
	bool isLeaf;
	friend class BTree;
public:
	Node(): n(0) {}
};

class BTree {
	Node root;
	int amount; //количество ключей

public:
	BTree(){
		Node newRoot;
		newRoot.isLeaf = true;
		root = newRoot;
		amount = 2;
	}

	void split_child(Node& x, Node& y, int index) { //делим узел у на две части и записываем их предку х а z - новый ребенок
		Node z;
		z.isLeaf = y.isLeaf;
		z.n = amount-1;
		for (int i=0; i<amount; i++) 
			z.keys.push_back(y.keys[i+amount]);
		if (!y.isLeaf)
			for (int i=0; i<amount+1; i++)
				z.children.push_back(y.children[i+amount]);
		y.n = amount-1;
		int i=0;
		auto it = x.children.begin();
		while (i<index) {
				++it;
				i++;
		}
		x.children.insert(it,z);
		x.n++;
	}

	void tree_insert_nonfull(Node& x, int new1) {
		int i = 0;
		auto it = x.keys.begin();
		if (x.isLeaf) {
			while ((i<x.n) && (new1 > x.keys[i])) {
				++it;
				i++;
			}
			x.keys.insert(it,new1);
			x.n++;
		}
		else {
			while ((i<x.n) && (new1 > x.keys[i])) i++;
			if ((*x.children[i]).n == 2*amount -1) {
				split_child(x, *x.children[i], new1);
				if (new1 > x.keys[i]) i++;
			}
			tree_insert_nonfull((*x.children[i]), new1);
		}
	}

	void tree_insert(int new1) {
		Node r = root;
		if (root.n = 2*amount-1) {
			Node newRoot;
			root = newRoot;
			newRoot.isLeaf = false;
			*newRoot.children[0] = r;
			split_child(root, r, 1);
			tree_insert_nonfull(root, new1);
		}
		else tree_insert_nonfull(root,new1);
	}
}

int main() {
	return 0;
}