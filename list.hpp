template<typename NodeTy, size_t N>
NodeTy* init_node_list(NodeTy(&node_arr)[N])
{
	size_t i = 0;
	for (; i < N - 1; ++i)
	{
		node_arr[i].next = node_arr + i + 1;
	}
	node_arr[i].next = nullptr;
	return node_arr;
	/*for (; i > 0; --i)
	{
	node_arr[i]->prev() = node_arr + i - 1;
	}
	node_arr[i]->prev() = nullptr;*/
}

template<typename NodeTy>
void move_si_to_other(NodeTy*& head, NodeTy*& tail, NodeTy*& other)
{
	if (head)
	{
		tail->next = other;
		other = head;
		head = tail = nullptr;
	}
}

template<typename NodeTy>
void add_si_head(NodeTy* node, NodeTy*& head)
{
	node->next = head;
	head = node;
}

template<typename NodeTy>
void add_si_tail(NodeTy* node, NodeTy*& tail)
{
	node->prev = tail;
	tail = node;
}

template<typename NodeTy>
NodeTy* remove_si_head(NodeTy*& head)
{
	if (head)
	{
		NodeTy* ret = head;
		head = head->next;
		return ret;
	}
	return nullptr;
}

template<class NodeTy>
NodeTy* remove_si_tail(NodeTy*& tail)
{
	if (tail)
	{
		NodeTy* ret = tail;
		tail = tail->prev;
		return ret;
	}
	return nullptr;
}

template<typename NodeTy>
void add_dual_first(NodeTy* node, NodeTy*& head, NodeTy*& tail)
{
	node->prev = nullptr;
	node->next = head;
	if (head)
	{
		head->prev = node;
	}
	else
	{
		tail = node;
	}
	head = node;
}

template<typename NodeTy>
void add_dual_last(NodeTy* node, NodeTy*& head, NodeTy*& tail)
{
	node->next = nullptr;
	node->prev = tail;
	if (tail)
	{
		tail->next = node;
	}
	else
	{
		head = node;
	}
	tail = node;
}

template<typename NodeTy>
NodeTy* remove_dual_first(NodeTy*& head, NodeTy*& tail)
{
	if (head)
	{
		NodeTy* ret = head;
		head = head->next;
		if (head)
		{
			head->prev = nullptr;
		}
		else
		{
			tail = nullptr;
		}
		return ret;
	}
	return nullptr;
}

template<typename NodeTy>
NodeTy* remove_list_last(NodeTy*& head, NodeTy*& tail)
{
	if (tail)
	{
		NodeTy* ret = tail;
		tail = tail->prev;
		if (tail)
		{
			tail->next = nullptr;
		}
		else
		{
			head = nullptr;
		}
		return ret;
	}
	return nullptr;
}

template<typename NodeTy>
void remove_list_node(NodeTy* node, NodeTy*& head, NodeTy*& tail)
{
	NodeTy* next = node->next;
	if (next)
	{
		next->prev = node->prev;
	}
	else
	{
		tail = node->prev;
	}
	NodeTy* prev = node->prev;
	if (prev)
	{
		prev->next = node->next;
	}
	else
	{
		head = node->next;
	}
}
