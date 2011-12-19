#include <Python.h>
#include <structmember.h>

staticforward PyTypeObject SLListNodeType;
staticforward PyTypeObject SLListIteratorType;

typedef struct
{
    PyObject_HEAD
    PyObject* value;
    PyObject* next;
    PyObject* list_weakref;
} SLListNodeObject;


static SLListNodeObject* sllistnode_create(PyObject* next,
                                           PyObject* value,
                                           PyObject* owner_list)
{
    SLListNodeObject* node;
    assert(value != NULL);

    node = (SLListNodeObject*)PyObject_CallObject(
                                                  (PyObject*)&SLListNodeType, NULL);

    if (next != NULL && next != Py_None)
        node->next = next;

    assert(node->value == Py_None);

    Py_INCREF(value);
    node->value = value;
    node->list_weakref = PyWeakref_NewRef(owner_list, NULL);

    return node;
}


static void sllistnode_dealloc(SLListNodeObject* self)
{
    Py_DECREF(Py_None);
    Py_DECREF(self->value);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject* sllistnode_new(PyTypeObject* type,
                                PyObject* args,
                                PyObject* kwds)
{
    SLListNodeObject* self;

    self = (SLListNodeObject*)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;


    Py_INCREF(Py_None);

    self->next = Py_None;
    self->value = Py_None;
    self->list_weakref = Py_None;
    return (PyObject*)self;
}

static PyObject* sllistnode_call(PyObject* self,
                                 PyObject* args,
                                 PyObject* kw)
{
    SLListNodeObject* node = (SLListNodeObject*)self;
    Py_INCREF(node->value);
    return node->value;
}




static PyMemberDef SLListNodeMembers[] =
    {
        { "value", T_OBJECT_EX, offsetof(SLListNodeObject, value), 0,
          "value" },
        { "next", T_OBJECT_EX, offsetof(SLListNodeObject, next), READONLY,
          "next node" },
        { NULL },   /* sentinel */
    };


static PyTypeObject SLListNodeType =
    {
        PyObject_HEAD_INIT(NULL)
        0,                              /* ob_size           */
        "llist.SLListNode",             /* tp_name           */
        sizeof(SLListNodeObject),       /* tp_basicsize      */
        0,                              /* tp_itemsize       */
        (destructor)sllistnode_dealloc, /* tp_dealloc        */
        0,                              /* tp_print          */
        0,                              /* tp_getattr        */
        0,                              /* tp_setattr        */
        0,                              /* tp_compare        */
        0,                              /* tp_repr           */
        0,                              /* tp_as_number      */
        0,                              /* tp_as_sequence    */
        0,                              /* tp_as_mapping     */
        0,                              /* tp_hash           */
        sllistnode_call,                /* tp_call           */
        0,                              /* tp_str            */
        0,                              /* tp_getattro       */
        0,                              /* tp_setattro       */
        0,                              /* tp_as_buffer      */
        Py_TPFLAGS_DEFAULT,             /* tp_flags          */
        "Singly linked list node",      /* tp_doc            */
        0,                              /* tp_traverse       */
        0,                              /* tp_clear          */
        0,                              /* tp_richcompare    */
        0,                              /* tp_weaklistoffset */
        0,                              /* tp_iter           */
        0,                              /* tp_iternext       */
        0,                              /* tp_methods        */
        SLListNodeMembers,              /* tp_members        */
        0,                              /* tp_getset         */
        0,                              /* tp_base           */
        0,                              /* tp_dict           */
        0,                              /* tp_descr_get      */
        0,                              /* tp_descr_set      */
        0,                              /* tp_dictoffset     */
        0,                              /* tp_init           */
        0,                              /* tp_alloc          */
        sllistnode_new,                 /* tp_new            */
    };




/* ****************************************************************************** */
/*                                      SLLIST                                    */
/* ****************************************************************************** */

staticforward PyTypeObject SLListType;
typedef struct
{
    PyObject_HEAD
    PyObject* first;
    PyObject* last;
    Py_ssize_t size;
    PyObject* weakref_list;
} SLListObject;


static void sllist_dealloc(SLListObject* self)
{
    PyObject* node = self->first;

    if (self->weakref_list != NULL)
        PyObject_ClearWeakRefs((PyObject*)self);

    while (node != Py_None)
    {
        PyObject* next_node = ((SLListNodeObject*)node)->next;
        Py_DECREF(node);
        node = next_node;
    }

    Py_DECREF(Py_None);

    self->ob_type->tp_free((PyObject*)self);

}



static PyObject* sllist_new(PyTypeObject* type,
                            PyObject* args,
                            PyObject* kwds)
{
    SLListObject* self;

    self = (SLListObject*)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(Py_None);

    self->first = Py_None;
    self->last = Py_None;
    self->weakref_list = Py_None;
    self->size = 0;

    return (PyObject*)self;
}

static int sllist_extend(SLListObject* self, PyObject* sequence)
{
    Py_ssize_t i;
    Py_ssize_t sequence_len;

    sequence_len = PySequence_Length(sequence);
    if (sequence_len == -1)
        {
            PyErr_SetString(PyExc_ValueError, "Invalid sequence");
            return 0;
        }

    for (i = 0; i < sequence_len; ++i)
        {
            PyObject* item;
            PyObject* new_node;

            item = PySequence_GetItem(sequence, i);
            if (item == NULL)
                {
                    PyErr_SetString(PyExc_ValueError,
                                    "Failed to get element from sequence");
                    return 0;
                }

            new_node = (PyObject*)sllistnode_create(Py_None,
                                                    item,
                                                    (PyObject*)self);


            if(self->first == Py_None)
                self->first = (PyObject*)new_node;
            else
                ((SLListNodeObject*)self->last)->next = (PyObject*)new_node;

            self->last = (PyObject*)new_node;

            ++self->size;
            Py_DECREF(item);
        }

    return 1;
}



static int sllist_init(SLListObject* self, PyObject* args, PyObject* kwds)
{
    PyObject* sequence = NULL;

    if (!PyArg_UnpackTuple(args, "__init__", 0, 1, &sequence))
        return -1;

    if (sequence == NULL)
        return 0;

    /* initialize list using passed sequence */
    if (!PySequence_Check(sequence))
        {
            PyErr_SetString(PyExc_TypeError, "Argument must be a sequence");
            return -1;
        }

    return sllist_extend(self, sequence) ? 0 : -1;
}


static SLListNodeObject* sllist_get_prev(SLListObject* self,
                                         SLListNodeObject* next)
{
    SLListNodeObject* prev = NULL;

    SLListNodeObject* node = (SLListNodeObject*)self->first;

    if (!PyObject_TypeCheck(next, &SLListNodeType)) {
        PyErr_SetString(PyExc_TypeError, "Not a SLLNodeObject");
        return NULL;
    }

    if (self->first == Py_None) {
        PyErr_SetString(PyExc_RuntimeError, "List is empty");
        return NULL;
    }

    if (self->first == (PyObject*)next) {
        PyErr_SetString(PyExc_RuntimeError, "No Previous elements");
        return NULL;
    }

    if(self->first != Py_None) {
        while(node != Py_None && node != next){
            prev = node;
            node = node->next;
        }
        Py_INCREF(prev);
        return (SLListNodeObject*)prev;
    }
    return NULL;

}


static PyObject* sllist_appendleft(SLListObject* self, PyObject* arg)
{
    if (!PyObject_TypeCheck(arg, &SLListNodeType)) {
        PyErr_SetString(PyExc_TypeError, "not a SLLNodeObject");
        return NULL;
    }

    SLListNodeObject* new_node = sllistnode_create(self->first,
                                                   ((SLListNodeObject*)arg)->value,
                                                   (PyObject*)self);

    self->first  = (PyObject*)new_node;

    if(self->last == Py_None)
        self->last = (PyObject*)new_node;

    ++self->size;

    Py_INCREF((PyObject*)new_node);
    return (PyObject*)new_node;
}

static PyObject* sllist_appendright(SLListObject* self, PyObject* arg)
{

    if (!PyObject_TypeCheck(arg, &SLListNodeType)) {
        PyErr_SetString(PyExc_TypeError, "not a SLLNodeObject");
        return NULL;
    }
    SLListNodeObject* new_node = sllistnode_create(Py_None,
                                                   ((SLListNodeObject*)arg)->value,
                                                   (PyObject*)self);

    if(self->first == Py_None)
        self->first = (PyObject*)new_node;
    else
        ((SLListNodeObject*)self->last)->next = (PyObject*)new_node;

    self->last = (PyObject*)new_node;

    ++self->size;
    Py_INCREF((PyObject*)new_node);
    return (PyObject*)new_node;
}

static PyObject* sllist_insert_after(SLListObject* self, PyObject* args)
{

    PyObject* value = NULL;
    PyObject* before = NULL;
    SLListNodeObject* new_node;

    if (!PyArg_UnpackTuple(args, "insert", 1, 2, &value, &before))
        return NULL;

    if (!PyObject_TypeCheck(before, &SLListNodeType)) {
        PyErr_SetString(PyExc_TypeError, "Not a SLLNodeObject");
        return NULL;
    }

    new_node = sllistnode_create(Py_None,
                                 value,
                                 (PyObject*)self);

    if(before == Py_None)
        self->first = (PyObject*)new_node;
    else{
        new_node->next = ((SLListNodeObject*)before)->next;
        ((SLListNodeObject*)before)->next = (PyObject*)new_node;
    }
    ++self->size;
    Py_INCREF((PyObject*)new_node);
    return (PyObject*)new_node;
}


static SLListNodeObject* sllist_get_node_at(SLListObject* self,
                                            Py_ssize_t pos)
{
    SLListNodeObject* node;
    long counter;

    if(pos >= self->size || pos < 0){
        PyErr_SetString(PyExc_IndexError, "No such index");
        return NULL;
    }

    node = (SLListNodeObject*)self->first;
    assert((PyObject*)node != Py_None);
    for (counter = 0; counter < pos; ++counter)
        node = (SLListNodeObject*)node->next;

    return node;
}


static PyObject* sllist_get_item(PyObject* self, Py_ssize_t index)
{
    SLListNodeObject* node;

    node = sllist_get_node_at((SLListObject*)self, index);
    Py_XINCREF(node);

    return (PyObject*)node;
}

static int sllist_set_item(PyObject* self, Py_ssize_t index, PyObject* val)
{
    SLListObject* list = (SLListObject*)self;

    if (!PyObject_TypeCheck(val, &SLListNodeType)) {
        PyErr_SetString(PyExc_TypeError, "Not a SLLNodeObject");
        return -1;
    }
    SLListNodeObject* node;
    if(index==0)
        node = (SLListNodeObject*)list->first;
    else if(index==list->size-1)
        node = (SLListNodeObject*)list->last;
    else
        node = (SLListNodeObject*)sllist_get_node_at(list, index);
    Py_XDECREF(node->value);
    node->value =((SLListNodeObject*)val)->value;
    return 0;
}

static PyObject* sllist_popleft(SLListObject* self)
{
    SLListNodeObject* del_node;

    if (self->first == Py_None)
        {
            PyErr_SetString(PyExc_RuntimeError, "List is empty");
            return NULL;
        }

    del_node = (SLListNodeObject*)self->first;

    self->first = del_node->next;
    if (self->last == (PyObject*)del_node)
        self->last = Py_None;

    --self->size;

    Py_DECREF((PyObject*)del_node);
    Py_RETURN_NONE;
}


static PyObject* sllist_popright(SLListObject* self)
{

    SLListNodeObject* del_node;
    SLListNodeObject* node;

    if (self->last == Py_None)
        {
            PyErr_SetString(PyExc_RuntimeError, "List is empty");
            return NULL;
        }

    del_node = (SLListNodeObject*)self->last;
    node = (SLListNodeObject*)self->first;


    if (self->first == (PyObject*)del_node)
        self->last = Py_None;
    else {
        self->last = (PyObject*)sllist_get_prev(self, (SLListNodeObject*)del_node);
    }
    --self->size;

    Py_DECREF((PyObject*)del_node);
    Py_RETURN_NONE;
}




static PyObject* sllist_remove(SLListObject* self, PyObject* arg)
{
    if (!PyObject_TypeCheck(arg, &SLListNodeType)) {
        PyErr_SetString(PyExc_TypeError, "Argument is not a SLLNodeObject");
        return NULL;
    }

    SLListNodeObject* prev;
    PyObject* list_ref;

    if (self->first == Py_None) {
        PyErr_SetString(PyExc_RuntimeError, "List is empty");
        return NULL;
    }

    list_ref = PyWeakref_GetObject(((SLListNodeObject*)arg)->list_weakref);
    if (list_ref != (PyObject*)self)
    {
        PyErr_SetString(PyExc_RuntimeError,
            "SLListNode belongs to another list");
        return NULL;
    }

    if(self->first == arg)
        self->first = ((SLListNodeObject*)arg)->next;
    if(self->last == arg)
        self->last = Py_None;

    if(self->first != Py_None) {
        prev = sllist_get_prev(self, (SLListNodeObject*)arg);
        if(arg == Py_None)
            Py_RETURN_NONE;
        prev->next = ((SLListNodeObject*)arg)->next;
        self->last = (PyObject*)prev;

    }
    Py_DECREF(arg);

    --self->size;
    Py_RETURN_NONE;

}

static PyObject* sllist_to_string(SLListObject* self,
                                  reprfunc fmt_func)
{
    PyObject* str = NULL;
    PyObject* comma_str = NULL;
    PyObject* tmp_str;
    SLListNodeObject* node = (SLListNodeObject*)self->first;

    assert(fmt_func != NULL);

    if (self->first == Py_None)
    {
        str = PyString_FromString("SLList()");
        if (str == NULL)
            goto str_alloc_error;
        return str;
    }

    str = PyString_FromString("SLList([");
    if (str == NULL)
        goto str_alloc_error;

    comma_str = PyString_FromString(", ");
    if (comma_str == NULL)
        goto str_alloc_error;

    while ((PyObject*)node != Py_None)
    {
        if (node != (SLListNodeObject*)self->first)
            PyString_Concat(&str, comma_str);

        tmp_str = fmt_func(node->value);
        if (tmp_str == NULL)
            goto str_alloc_error;
        PyString_ConcatAndDel(&str, tmp_str);

        node = (SLListNodeObject*)node->next;
    }

    Py_DECREF(comma_str);
    comma_str = NULL;

    tmp_str = PyString_FromString("])");
    if (tmp_str == NULL)
        goto str_alloc_error;
    PyString_ConcatAndDel(&str, tmp_str);

    return str;

str_alloc_error:
    Py_XDECREF(str);
    Py_XDECREF(comma_str);
    PyErr_SetString(PyExc_RuntimeError, "Failed to create string");
    return NULL;
}


static PyObject* sllist_repr(SLListObject* self)
{
    return sllist_to_string(self, PyObject_Repr);
}

static PyObject* sllist_str(SLListObject* self)
{
    return sllist_to_string(self, PyObject_Str);
}


static Py_ssize_t sllist_len(PyObject* self)
{
    return ((SLListObject*)self)->size;
}

static PyMethodDef SLListMethods[] =
    {
        { "appendleft", (PyCFunction)sllist_appendleft, METH_O,
          "Append element at the beginning of the list" },

        { "appendright", (PyCFunction)sllist_appendright, METH_O,
          "Append element at the end of the list" },

        { "append", (PyCFunction)sllist_appendright, METH_O,
          "Append element at the end of the list" },

        { "insert_after", (PyCFunction)sllist_insert_after, METH_VARARGS,
          "Inserts element after node" },

        { "get_prev", (PyCFunction)sllist_get_prev, METH_VARARGS,
          "Get prevoius node for given one" },

        { "pop", (PyCFunction)sllist_popleft, METH_NOARGS,
          "Remove last element from the list and return it" },

        { "popleft", (PyCFunction)sllist_popleft, METH_NOARGS,
          "Remove first element from the list and return it" },

        { "popright", (PyCFunction)sllist_popright, METH_NOARGS,
          "Remove last element from the list and return it" },

        { "remove", (PyCFunction)sllist_remove, METH_O,
          "Remove element from the list" },

        { NULL },   /* sentinel */
    };


static PyMemberDef SLListMembers[] =
    {
        { "first", T_OBJECT_EX, offsetof(SLListObject, first), READONLY,
          "First node" },
        { "last", T_OBJECT_EX, offsetof(SLListObject, last), READONLY,
          "Next node" },
        { "size", T_INT, offsetof(SLListObject, size), 0,
          "size" },

        { NULL },   /* sentinel */
    };

static PySequenceMethods SLListSequenceMethods[] =
    {
        sllist_len,                  /* sq_length         */
        0,                           /* sq_concat         */
        0,                           /* sq_repeat         */
        sllist_get_item,             /* sq_item           */
        0,                           /* sq_slice;         */
        sllist_set_item,             /* sq_ass_item       */
        0                            /* sq_ass_slice      */
    };

static PyTypeObject SLListType =
    {
        PyObject_HEAD_INIT(NULL)
        0,                           /* ob_size           */
        "llist.SLList",              /* tp_name           */
        sizeof(SLListObject),        /* tp_basicsize      */
        0,                           /* tp_itemsize       */
        (destructor)sllist_dealloc,  /* tp_dealloc        */
        0,                           /* tp_print          */
        0,                           /* tp_getattr        */
        0,                           /* tp_setattr        */
        0,                           /* tp_compare        */
        (reprfunc)sllist_repr,       /* tp_repr           */
        0,                           /* tp_as_number      */
        SLListSequenceMethods,       /* tp_as_sequence    */
        0,                           /* tp_as_mapping     */
        0,                           /* tp_hash           */
        0,                           /* tp_call           */
        (reprfunc)sllist_str,        /* tp_str            */
        0,                           /* tp_getattro       */
        0,                           /* tp_setattro       */
        0,                           /* tp_as_buffer      */
        Py_TPFLAGS_DEFAULT,          /* tp_flags          */
        "Singly linked list",        /* tp_doc            */
        0,                           /* tp_traverse       */
        0,                           /* tp_clear          */
        0,                           /* tp_richcompare    */
        0,                           /* tp_weaklistoffset */
        0,                           /* tp_iter           */
        0,                           /* tp_iternext       */
        SLListMethods,               /* tp_methods        */
        SLListMembers,               /* tp_members        */
        0,                           /* tp_getset         */
        0,                           /* tp_base           */
        0,                           /* tp_dict           */
        0,                           /* tp_descr_get      */
        0,                           /* tp_descr_set      */
        0,                           /* tp_dictoffset     */
        (initproc)sllist_init,       /* tp_init           */
        0,                           /* tp_alloc          */
        sllist_new,                  /* tp_new            */
    };


typedef struct
{
    PyObject_HEAD
    SLListObject* list;
    PyObject* current_node;
} SLListIteratorObject;

static void sllistiterator_dealloc(SLListIteratorObject* self)
{
    Py_XDECREF(self->current_node);
    Py_DECREF(self->list);

    self->ob_type->tp_free((PyObject*)self);
}

static PyObject* sllistiterator_new(PyTypeObject* type,
                                    PyObject* args,
                                    PyObject* kwds)
{
    SLListIteratorObject* self;
    PyObject* owner_list = NULL;

    if (!PyArg_UnpackTuple(args, "__new__", 1, 1, &owner_list))
        return NULL;

    if (!PyObject_TypeCheck(owner_list, &SLListType))
    {
        PyErr_SetString(PyExc_TypeError, "SLList argument expected");
        return NULL;
    }

    self = (SLListIteratorObject*)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->list = (SLListObject*)owner_list;
    self->current_node = self->list->first;

    Py_INCREF(self->list);
    Py_INCREF(self->current_node);

    return (PyObject*)self;
}


static PyObject* sllistiterator_iternext(PyObject* self)
{
    SLListIteratorObject* iter_self = (SLListIteratorObject*)self;
    PyObject* value;
    PyObject* next_node;

    if (iter_self->current_node == NULL || iter_self->current_node == Py_None)
    {
        Py_XDECREF(iter_self->current_node);
        iter_self->current_node = NULL;
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    value = ((SLListNodeObject*)iter_self->current_node)->value;
    Py_INCREF(value);

    next_node = ((SLListNodeObject*)iter_self->current_node)->next;
    Py_INCREF(next_node);
    Py_DECREF(iter_self->current_node);
    iter_self->current_node = next_node;

    return value;
}



static PyTypeObject SLListIteratorType =
{
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "llist.SLListIterator",             /* tp_name */
    sizeof(SLListIteratorObject),       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)sllistiterator_dealloc, /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    "Singly linked list iterator",      /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    sllistiterator_iternext,            /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    sllistiterator_new,                 /* tp_new */
};



int sllist_init_type()
{
    return
        ((PyType_Ready(&SLListType) == 0) &&
         (PyType_Ready(&SLListNodeType) == 0) &&
         (PyType_Ready(&SLListIteratorType) == 0))
        ? 1 : 0;
}

void sllist_register(PyObject* module)
{
    Py_INCREF(&SLListType);
    Py_INCREF(&SLListNodeType);
    Py_INCREF(&SLListIteratorType);

    PyModule_AddObject(module, "SLList", (PyObject*)&SLListType);
    PyModule_AddObject(module, "SLListNode", (PyObject*)&SLListNodeType);
    PyModule_AddObject(module, "SLListIterator", (PyObject*)&SLListIteratorType);
}
