#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <node_data.h>
#include <vector.h>



#define VECTOR_TYPE_ID      551087
#define VECTOR_DEFAULT_SIZE 10


struct vector_struct {
  UTIL_TYPE_ID_DECLARATION;
  int    	   alloc_size;   /* The number of elements allocated in the data vector - in general > size. */
  int    	   size;         /* THe number of elements the user has added to the vector. */
  node_data_type **data;         /* node_data instances - which again contain user data. */
};

 

/* Small datastructure PURELY used for sorting the vector. */

typedef struct {
  vector_cmp_ftype * user_cmp;
  node_data_type   * data;
} vector_sort_node_type;
  

UTIL_SAFE_CAST_FUNCTION(vector , VECTOR_TYPE_ID)



static void vector_resize__(vector_type * vector, int new_alloc_size) {
  int i;
  if (new_alloc_size < vector->alloc_size) {
    /* The vector is shrinking. */
    for (i=new_alloc_size; i < vector->alloc_size; i++)
      node_data_free( vector->data[i] );
  } 
  
  vector->data = util_realloc( vector->data , new_alloc_size * sizeof * vector->data , __func__);  
  for (i = vector->alloc_size; i < new_alloc_size; i++)
    vector->data[i] = NULL; /* nitialising new nodes to NULL */

  vector->alloc_size = new_alloc_size;
}


vector_type * vector_alloc_new() {
  vector_type * vector = util_malloc( sizeof * vector , __func__);
  UTIL_TYPE_ID_INIT(vector , VECTOR_TYPE_ID);
  vector->size         = 0;
  vector->alloc_size   = 0;
  vector->data         = NULL;
  vector_resize__(vector , VECTOR_DEFAULT_SIZE);
  return vector;
}


/**
   This functon will allocate a vector 'size' elements. Each of these
   elements is initialized with NULL pointers.
*/

vector_type * vector_alloc_NULL_initialized( int size ) { 
  vector_type * vector = vector_alloc_new();
  for (int i=0; i < size; i++)
    vector_append_ref( vector , NULL );
  return vector;
}


static int vector_append_node(vector_type * vector , node_data_type * node);

/** 
    This function assumes that the index is *inside* the vector,
    otherwise it will fail HARD. Should NOT be exported (then we
    suddenly have to check for 'holes' in the vector.

    If (index == vector->size) i.e. this amounts to an append, the
    append routine will be called (which again will call this function
    ...) ; if index > vector->size, the function will fail hard.
*/

static void vector_iset__(vector_type * vector , int index , node_data_type * node) {
  if (index == vector->size) 
    vector_append_node( vector , node );
  else if (index > vector->size) 
    util_abort("%s: called with index:%d  max_value:%d \n",__func__ , index , vector->size);
  else {
    if (vector->data[index] != NULL) 
      node_data_free( vector->data[index] );

    vector->data[index] = node;
  }
}

/**
   This is the low level function opposite to the vector_idel()
   function. A new value (node) is inserted at index, and the rest of
   the vector is shifted to the right.
*/

static void vector_insert__(vector_type * vector , int index , node_data_type * node) {
  if (vector->size == vector->alloc_size)
    vector_resize__(vector , 2*(vector->alloc_size + 1));
  {
    int bytes_to_move = (vector->size - index) * sizeof * vector->data;
    memmove(&vector->data[index + 1] , &vector->data[index] , bytes_to_move);
    vector->data[index] = NULL;   /* Otherwise the destructor might try to pick up on it in the vector_iset__() call below */
  }
  vector->size++;
  vector_iset__( vector , index , node );
}


/**
   This is the low-level append node function which actually "does
   it", the node has been allocated in one of the front-end
   functions. The return value is the index of the node (which can
   subsequently be used in a vector_iget() call)).
*/
static int vector_append_node(vector_type * vector , node_data_type * node) {
  if (vector->size == vector->alloc_size)
    vector_resize__(vector , 2*(vector->alloc_size + 1));
  
  vector->size++;
  vector_iset__(vector , vector->size - 1 , node);
  return vector->size - 1;
}


/*
  This is like the vector_append_node() function, but the node is
  pushed in at the front.
*/

static void vector_push_node(vector_type * vector , node_data_type * node) {
  if (vector->size == vector->alloc_size)
    vector_resize__(vector , 2*(vector->alloc_size + 1));
  {
    int bytes = vector->size * sizeof * vector->data;
    if (bytes > 0) {
      memmove(&vector->data[1] , vector->data , bytes);
      vector->data[0] = NULL;   /* Otherwise the destructor might try to pick up on it in the vector_iset__() call below */
    }
  }
  vector->size++;
  vector_iset__(vector , 0 , node);
}



/**
   Append a user-pointer which comes without either copy constructor
   or destructor, this implies that the calling scope has FULL
   responsabilty for the storage of the data added to the vector.
*/


int vector_append_ref(vector_type * vector , const void * data) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , NULL);
  return vector_append_node(vector , node);
}

void vector_push_ref(vector_type * vector , const void * data) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , NULL);
  vector_push_node(vector , node);
}

void vector_iset_ref(vector_type * vector , int index , const void * data) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , NULL);
  vector_iset__(vector , index , node);
}

void vector_insert_ref(vector_type * vector , int index , const void * data) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , NULL);
  vector_insert__(vector , index , node);
}



/**
   Append a user-pointer which the vector instance takes ownership
   of. This means that when the vector is destroyed it calls the
   destructor on the data which has been supplied. The calling scope
   should basically let this object be - the vector has taken control.
*/


int vector_append_owned_ref(vector_type * vector , const void * data , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , del);
  return vector_append_node(vector , node);
}

void vector_push_owned_ref(vector_type * vector , const void * data , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , del);
  vector_push_node(vector , node);
}


void vector_iset_owned_ref(vector_type * vector , int index , const void * data , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , del);
  vector_iset__(vector , index , node);
}


void vector_insert_owned_ref(vector_type * vector , int index , const void * data , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, NULL , del);
  vector_insert__(vector , index , node);
}


/**
  This function appends a COPY of user object. This implies that the
  calling scope is still responsible for the instance declared and
  used in that scope, whereas the vector takes responsability of
  freeing it's own copy.
*/


int  vector_append_copy(vector_type * vector , const void * data , copyc_ftype * copyc , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, copyc , del);
  return vector_append_node(vector , node);
}

void  vector_push_copy(vector_type * vector , const void * data , copyc_ftype * copyc , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, copyc , del);
  return vector_push_node(vector , node);
}


void vector_iset_copy(vector_type * vector , int index , const void * data , copyc_ftype * copyc , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, copyc , del);
  vector_iset__(vector , index , node);
}

void vector_insert_copy(vector_type * vector , int index , const void * data , copyc_ftype * copyc , free_ftype * del) {
  node_data_type * node = node_data_alloc_ptr( data, copyc , del);
  vector_insert__(vector , index , node);
}



/**
   A buffer is unstructured storage (i.e. a void *) which is destroyed
   with free, and copied with malloc + memcpy. The vector takes a copy
   of the buffer which is inserted (and freed on vector destruction).
*/
   

void vector_append_buffer(vector_type * vector , const void * buffer, int buffer_size) {
  node_data_type * node = node_data_alloc_buffer( buffer , buffer_size );
  vector_append_node(vector , node);
}


void vector_iset_buffer(vector_type * vector , int index , const void * buffer, int buffer_size) {
  node_data_type * node = node_data_alloc_buffer( buffer , buffer_size );
  vector_iset__(vector , index , node);
}

void vector_push_buffer(vector_type * vector , const void * buffer, int buffer_size) {
  node_data_type * node = node_data_alloc_buffer( buffer , buffer_size );
  vector_push_node(vector , node);
}


const void * vector_iget_const(const vector_type * vector, int index) {
  if ((index >= 0) && (index < vector->size)) {
    const node_data_type * node = vector->data[index];
    return node_data_get_ptr( node );
  } else {
    util_abort("%s: Invald index:%d  Valid range: [0,%d> \n",__func__ , index , vector->size);
    return NULL;
  }
}


void * vector_iget(const vector_type * vector, int index) {
  if ((index >= 0) && (index < vector->size)) {
    const node_data_type * node = vector->data[index];
    return node_data_get_ptr( node );
  } else {
    util_abort("%s: Invald index:%d  Valid range: [0,%d> \n",__func__ , index , vector->size);
    return NULL;
  }
}



/* 
   Removes element nr index from the vector, if a destructor is
   associated with element 'index' it is called, and the memory
   freed. Afterwards all elements at positions (index +1) and onwards
   are shifted one element to the left. 
*/


void vector_idel(vector_type * vector , int index) {
  if ((index >= 0) && (index < vector->size)) {
    node_data_type * node = vector->data[index];
    node_data_free( node );  /* Discard the element */
    {
      int bytes_to_move = (vector->size - 1 - index) * sizeof * vector->data;
      memmove(&vector->data[index] , &vector->data[index + 1] , bytes_to_move);
      vector->data[vector->size - 1] = NULL;  /* Clear the last element  - which is no longer valid. */
      vector->size--;
    }
  } else 
    util_abort("%s: Invald index:%d  Valid range: [0,%d> \n",__func__ , index , vector->size);
}





/** 
    Will abort if the vector is empty. 
*/
void * vector_get_last(const vector_type * vector) {
  if (vector->size == 0)
    util_abort("%s: asking to get the last element in an empty vector - impossible ... \n",__func__);
  {
    const node_data_type * node = vector->data[vector->size - 1];
    return node_data_get_ptr( node );
  }
}


/** 
    Will abort if the vector is empty. 
*/
const void * vector_get_last_const(const vector_type * vector) {
  if (vector->size == 0)
    util_abort("%s: asking to get the last element in an empty vector - impossible ... \n",__func__);
  {
    const node_data_type * node = vector->data[vector->size - 1];
    return node_data_get_ptr( node );
  }
}


/**
   This function removes the last element from the vector and returns
   it to the calling scope. Irrespective of whether the element _was_
   inserted with a destructor: when calling vector_pop() the calling
   scope takes responsability for freeing data.
*/


void * vector_pop(vector_type * vector) {
  if (vector->size == 0)
    util_abort("%s: asking to get the last element in an empty vector - impossible ... \n",__func__);
  {
    node_data_type * node = vector->data[vector->size - 1];
    void * data = node_data_get_ptr( node );
    
    node_data_free_container( node );  /* Free the container holding data. */
    vector->size--;                    /* Shrink the vector */
    return data;
  }
}


/* 
   Removes the first element from the vector and returns it - similar
   to vector_pop():
*/

void * vector_unshift(vector_type * vector ) {
  if (vector->size == 0)
    util_abort("%s: asking to get the last element in an empty vector - impossible ... \n",__func__);
  {
    node_data_type * node = vector->data[vector->size - 1];
    void * data = node_data_get_ptr( node );
    
    node_data_free_container( node );  /* Free the container holding data. */
    {
      int bytes = (vector->size - 1) * sizeof * vector->data;  /* Move the storage one element to  the left (could als be implemented with an offset??). */
      memmove( vector->data , &vector->data[1] , bytes);
    }
    vector->size--;                    /* Shrink the vector */
    return data;
  }
}



int vector_get_size( const vector_type * vector) {
  return vector->size;
}


/**
   This vector frees all the storage of the vector, including all the
   nodes which have been installed with a destructor.
*/

void vector_clear(vector_type * vector) {
  int i;
  for (i = 0; i < vector->size; i++) {
    node_data_free(vector->data[i]);  /* User specific destructors are called here. */
    vector->data[i] = NULL;           /* This is essential to protect against unwaranted calls to destructors when data is reused. */
  }
  vector->size = 0;
}


void vector_free(vector_type * vector) {
  vector_clear( vector );
  free(vector->data);
  free(vector);
}


void vector_free__( void * arg ) {
  vector_free( vector_safe_cast( arg ));
}


static int vector_cmp(const void * s1 , const void * s2) {
  const vector_sort_node_type * node1 = (const vector_sort_node_type *) s1;
  const vector_sort_node_type * node2 = (const vector_sort_node_type *) s2;
    
  return node1->user_cmp(node_data_get_ptr(node1->data) , node_data_get_ptr(node2->data));
}


/**
   This function will sort the vector content in place. The sorting is
   based on a user-supplied cmp function which should return -1,0,1
   when comparing two elements. The prototype of this function is

       int (* user_cmp) (const void *, const void *)

   i.e. the same as for qsort. The vector implementation considers
   (fully) untyped data, it is therefor the users responsability to
   ensure that the comparison makes sense. For example:


     double * p = util_malloc(10 * sizeof * p, __func__);
     vector_append_buffer(vector , "This is a string ..." , strlen());
     vector_append_buffer(vector , p , 10 * sizeof * p);

   Here we have inserted one (char *) and one (double *). When these
   elements arrive in the sort function they will just be (void *),
   and the comparison will be quite meaningless(??).
*/



void vector_sort(vector_type * vector , vector_cmp_ftype * cmp) {
  vector_sort_node_type * sort_data = util_malloc( vector->size * sizeof * sort_data , __func__);
  {
    int i;
    
    /* Fill up the temporary storage used for sorting */
    for (i = 0; i < vector->size; i++) {
      sort_data[i].data     = vector->data[i];
      sort_data[i].user_cmp = cmp;
    }
    
    /* Sort the temporary vector */
    qsort(sort_data , vector->size , sizeof * sort_data ,  vector_cmp);
    
    /* Recover the sorted vector */
    for (i = 0; i < vector->size; i++) 
      vector->data[i] = sort_data[i].data;
  }
  free( sort_data );
}


/*****************************************************************/


/*
  If deep_copy == true - all elements in the vector MUST have
  constructor, otherwise the node_data_alloc_copy() function will fail.
*/

vector_type * vector_alloc_copy(const vector_type * src , bool deep_copy) {
  vector_type * copy = vector_alloc_new();
  int i;
  for (i=0; i < src->size; i++) {
    node_data_type * node_copy = node_data_alloc_copy( src->data[i] , deep_copy);
    vector_append_node( copy , node_copy );
  }
  return copy;
}

