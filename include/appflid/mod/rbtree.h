#ifndef APPFLID_RBTREE_H
#define APPFLID_RBTREE_H

#define rbtree_find(expected, root, compare_cb, type, hook_name) \
		({ \
		 		type *result = NULL; \
		 		struct rb_node *node; \
		 		\
		 		node = (root)->rb_node; \
		 		while (node) { \
		 			type *entry = rb_entry(node, type, hook_name); \
		 			int comparison = compare_cb(entry, expected); \
		 			\
		 			if (comparison < 0) { \
		 				node = node->rb_left; \
		 			} else if (comparison > 0) { \
		 				node = node->rb_right; \
		 			} else { \
		 				result = entry; \
		 				break; \
		 			} \
		 		} \
		 		\
		 		result; \
		 	})

#define rbtree_add(entry, key, root, compare_cb, type, hook_name) \
		({ \
		 		struct rb_node **new = &((root)->rb_node), *parent = NULL; \
		 		int error = 0; \
		 		\
		 		/* Figure out where to put new node */ \
		 		while (*new) { \
		 			type *this = rb_entry(*new, type, hook_name); \
		 			int result = compare_cb(this, key); \
		 			\
		 			parent = *new; \
		 			if (result < 0) { \
		 				new = &((*new)->rb_left); \
		 			} else if (result > 0) { \
		 				new = &((*new)->rb_right); \
		 			} else { \
		 				error = -EEXIST; \
		 				break; \
		 			} \
		 		} \
		 		\
		 		/* Add new node and rebalance tree. */ \
					if (!error) { \
									rb_link_node(&(entry)->hook_name, parent, new); \
									rb_insert_color(&(entry)->hook_name, root); \
								} \
					\
					error; \
				})

/**
 *  * Destroys all of the tree's nodes.
 *   */
#define rbtree_clear(root, destructor) \
({ \
		/* ... using a postorder traversal. */ \
	struct rb_node *parent_hook, *current_hook; \
	\
	current_hook = rb_first(root); \
	\
	while (current_hook) { \
		while (current_hook->rb_right) { \
			current_hook = current_hook->rb_right; \
			while (current_hook->rb_left) \
  				current_hook = current_hook->rb_left; \
		} \
		\
		parent_hook = rb_parent(current_hook); \
		\
		if (parent_hook) { \
			if (current_hook == parent_hook->rb_left) \
				parent_hook->rb_left = NULL; \
			else /* if (current_hook == parent_hook->rb_right) */ \
				parent_hook->rb_right = NULL; \
		} \
		destructor(current_hook); \
		\
		current_hook = parent_hook; \
	} \
		\
	(root)->rb_node = NULL; \
})



#endif 
