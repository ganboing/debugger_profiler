/*





















*/

template<class NodeTy>
void __rb_rotate_left(NodeTy *node, typename NodeTy::RootTy *root)
{
	NodeTy *right = node->rb_right;
	NodeTy *parent = rb_parent(node);

	if ((node->rb_right = right->rb_left))
		rb_set_parent(right->rb_left, node);
	right->rb_left = node;

	rb_set_parent(right, parent);

	if (parent)
	{
		if (node == parent->rb_left)
			parent->rb_left = right;
		else
			parent->rb_right = right;
	}
	else
		root->rb_node = right;
	rb_set_parent(node, right);
}
template<class NodeTy>
void __rb_rotate_right(NodeTy *node, typename NodeTy::RootTy *root)
{
	NodeTy *left = node->rb_left;
	NodeTy *parent = rb_parent(node);

	if ((node->rb_left = left->rb_right))
		rb_set_parent(left->rb_right, node);
	left->rb_right = node;

	rb_set_parent(left, parent);

	if (parent)
	{
		if (node == parent->rb_right)
			parent->rb_right = left;
		else
			parent->rb_left = left;
	}
	else
		root->rb_node = left;
	rb_set_parent(node, left);
}
template<class NodeTy>
void rb_insert_color(NodeTy *node, typename NodeTy::RootTy *root)
{
	NodeTy *parent, *gparent;

	while ((parent = rb_parent(node)) && rb_is_red(parent))
	{
		gparent = rb_parent(parent);

		if (parent == gparent->rb_left)
		{
			{
				NodeTy *uncle = gparent->rb_right;
				if (uncle && rb_is_red(uncle))
				{
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rb_right == node)
			{
				NodeTy *tmp;
				__rb_rotate_left(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_right(gparent, root);
		} else {
			{
				NodeTy *uncle = gparent->rb_left;
				if (uncle && rb_is_red(uncle))
				{
					rb_set_black(uncle);
					rb_set_black(parent);
					rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rb_left == node)
			{
				NodeTy *tmp;
				__rb_rotate_right(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rb_set_black(parent);
			rb_set_red(gparent);
			__rb_rotate_left(gparent, root);
		}
	}

	rb_set_black(root->rb_node);
}

template<class NodeTy>
void __rb_erase_color(NodeTy *node, NodeTy *parent,
typename NodeTy::RootTy *root)
{
	NodeTy *other;

	while ((!node || rb_is_black(node)) && node != root->rb_node)
	{
		if (parent->rb_left == node)
		{
			other = parent->rb_right;
			if (rb_is_red(other))
			{
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_left(parent, root);
				other = parent->rb_right;
			}
			if ((!other->rb_left || rb_is_black(other->rb_left)) &&
			    (!other->rb_right || rb_is_black(other->rb_right)))
			{
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			}
			else
			{
				if (!other->rb_right || rb_is_black(other->rb_right))
				{
					rb_set_black(other->rb_left);
					rb_set_red(other);
					__rb_rotate_right(other, root);
					other = parent->rb_right;
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rb_right);
				__rb_rotate_left(parent, root);
				node = root->rb_node;
				break;
			}
		}
		else
		{
			other = parent->rb_left;
			if (rb_is_red(other))
			{
				rb_set_black(other);
				rb_set_red(parent);
				__rb_rotate_right(parent, root);
				other = parent->rb_left;
			}
			if ((!other->rb_left || rb_is_black(other->rb_left)) &&
			    (!other->rb_right || rb_is_black(other->rb_right)))
			{
				rb_set_red(other);
				node = parent;
				parent = rb_parent(node);
			}
			else
			{
				if (!other->rb_left || rb_is_black(other->rb_left))
				{
					rb_set_black(other->rb_right);
					rb_set_red(other);
					__rb_rotate_left(other, root);
					other = parent->rb_left;
				}
				rb_set_color(other, rb_color(parent));
				rb_set_black(parent);
				rb_set_black(other->rb_left);
				__rb_rotate_right(parent, root);
				node = root->rb_node;
				break;
			}
		}
	}
	if (node)
		rb_set_black(node);
}
template<class NodeTy>
void rb_erase(NodeTy *node, typename NodeTy::RootTy *root)
{
	NodeTy *child, *parent;
	bool color;

	if (!node->rb_left)
		child = node->rb_right;
	else if (!node->rb_right)
		child = node->rb_left;
	else
	{
		NodeTy *old = node, *left;

		node = node->rb_right;
		while ((left = node->rb_left) != nullptr)
			node = left;

		if (rb_parent(old)) {
			if (rb_parent(old)->rb_left == old)
				rb_parent(old)->rb_left = node;
			else
				rb_parent(old)->rb_right = node;
		} else
			root->rb_node = node;

		child = node->rb_right;
		parent = rb_parent(node);
		color = rb_color(node);

		if (parent == old) {
			parent = node;
		} else {
			if (child)
				rb_set_parent(child, parent);
			parent->rb_left = child;

			node->rb_right = old->rb_right;
			rb_set_parent(old->rb_right, node);
		}

		node->parent = old->parent, node->color = old->color;
		node->rb_left = old->rb_left;
		rb_set_parent(old->rb_left, node);

		goto color;
	}

	parent = rb_parent(node);
	color = rb_color(node);

	if (child)
		rb_set_parent(child, parent);
	if (parent)
	{
		if (parent->rb_left == node)
			parent->rb_left = child;
		else
			parent->rb_right = child;
	}
	else
		root->rb_node = child;

 color:
	if (color == RB_BLACK)
		__rb_erase_color(child, parent, root);
}


/*
 * This function returns the first node (in sort order) of the tree.
 */
template<class RootTy>
typename RootTy::NodeTy *rb_first(RootTy *root)
{
	typedef typename RootTy::NodeTy	NodeType; NodeType *n;

	n = root->rb_node;
	if (!n)
		return nullptr;
	while (n->rb_left)
		n = n->rb_left;
	return n;
}

template<class RootTy>
typename RootTy::NodeTy *rb_last(RootTy *root)
{
	typename RootTy::NodeTy	*n;

	n = root->rb_node;
	if (!n)
		return nullptr;
	while (n->rb_right)
		n = n->rb_right;
	return n;
}

template<class NodeTy>
NodeTy *rb_next(NodeTy *node)
{
	NodeTy *parent;

	if (rb_parent(node) == node)
		return nullptr;

	/* If we have a right-hand child, go down and then left as far
	   as we can. */
	if (node->rb_right) {
		node = node->rb_right; 
		while (node->rb_left)
			node=node->rb_left;
		return node;
	}

	/* No right-hand children.  Everything down and left is
	   smaller than us, so any 'next' node must be in the general
	   direction of our parent. Go up the tree; any time the
	   ancestor is a right-hand child of its parent, keep going
	   up. First time it's a left-hand child of its parent, said
	   parent is our 'next' node. */
	while ((parent = rb_parent(node)) && node == parent->rb_right)
		node = parent;

	return parent;
}

template<class NodeTy>
NodeTy *rb_prev(NodeTy *node)
{
	NodeTy *parent;

	if (rb_parent(node) == node)
		return nullptr;

	/* If we have a left-hand child, go down and then right as far
	   as we can. */
	if (node->rb_left) {
		node = node->rb_left; 
		while (node->rb_right)
			node=node->rb_right;
		return node;
	}

	/* No left-hand children. Go up till we find an ancestor which
	   is a right-hand child of its parent */
	while ((parent = rb_parent(node)) && node == parent->rb_left)
		node = parent;

	return parent;
}

template<class NodeTy>
void rb_replace_node(NodeTy *victim, NodeTy *_new,
typename NodeTy::RootTy *root)
{
	NodeTy *parent = rb_parent(victim);

	/* Set the surrounding nodes to point to the replacement */
	if (parent) {
		if (victim == parent->rb_left)
			parent->rb_left = _new;
		else
			parent->rb_right = _new;
	} else {
		root->rb_node = _new;
	}
	if (victim->rb_left)
		rb_set_parent(victim->rb_left, _new);
	if (victim->rb_right)
		rb_set_parent(victim->rb_right, _new);

	/* Copy the pointers/colour from the victim to the replacement */
	_new->rb_left = victim->rb_left;
	_new->rb_right = victim->rb_right;
	_new->parent = victim->parent;
	_new->color = victim->color;
}