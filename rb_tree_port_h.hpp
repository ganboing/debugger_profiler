#include <list>
#include <stdexcept>

#define	RB_RED		false
#define	RB_BLACK	true

template<class NodeTy>
NodeTy* rb_parent(NodeTy *r)
{
	return r->parent;
}

template<class NodeTy>
bool rb_color(NodeTy *r)
{
	return !!r->color;
}

template<class NodeTy>
bool rb_is_red(NodeTy *r)
{
	return !rb_color(r);
}

template<class NodeTy>
bool rb_is_black(NodeTy *r)
{
	return rb_color(r);
}

template<class NodeTy>
void rb_set_red(NodeTy *r)
{
	r->color = RB_RED;
}

template<class NodeTy>
void rb_set_black(NodeTy *r)
{
	r->color = RB_BLACK;
}

template<class NodeTy>
void rb_set_parent(NodeTy *rb, NodeTy *p)
{
	rb->parent = p;
}

template<class NodeTy>
void rb_set_color(NodeTy *rb, bool color)
{
	rb->color = color;
}

template<class NodeTy>
void rb_prep_new(NodeTy* node)
{
	node->color = RB_RED;
	node->rb_left = node->rb_right = nullptr;
}

template<class NodeTy>
void rb_link_node(NodeTy * node, NodeTy * parent,
	NodeTy** rb_link)
{
	node->color = RB_RED;
	node->parent = parent;
	node->rb_left = node->rb_right = nullptr;

	*rb_link = node;
}

template<class NodeTy>
size_t rb_check_subtree(NodeTy* node, NodeTy** list)
{
	size_t ret = 0;
	if (node)
	{
		if (rb_is_red(node))
		{
			if (*list == nullptr)
			{
				throw ::std::runtime_error("root is red");
			}
			if (rb_is_red(*list))
			{
				throw ::std::runtime_error("two red node");
			}
		}
		else
		{
			++ret;
		}
		*++list = node;
		const size_t left = rb_check_subtree<NodeTy>(node->rb_left, list);
		const size_t right = rb_check_subtree<NodeTy>(node->rb_right, list);
		if (left != right)
		{
			throw ::std::runtime_error("number of black nodes does not match");
		}
		ret += left;
		return ret;
	}
	return 0;
}

template<class NodeTy>
size_t rb_check_valid(const NodeTy* root)
{
	const NodeTy* verify_list[sizeof(size_t) * 8];
	verify_list[0] = nullptr;
	return rb_check_subtree(root, verify_list);
}

#include "rb_tree_port_c.hpp"
