/*
 * This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Authors: Heiko Strathmann, Viktor Gal, Soeren Sonnenburg, Soumyajit De,
 *          Yuyu Zhang, Thoralf Klein, Sergey Lisitsyn, Wu Lin
 */

#ifndef __INPUTPARSER_H__
#define __INPUTPARSER_H__

#include <shogun/lib/config.h>

#include <shogun/lib/common.h>
#include <shogun/io/SGIO.h>
#include <shogun/io/streaming/StreamingFile.h>
#include <shogun/io/streaming/ParseBuffer.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#define PARSER_DEFAULT_BUFFSIZE 100

namespace shogun
{
	/// Type of example, either E_LABELLED
	/// or E_UNLABELLED
	enum E_EXAMPLE_TYPE
	{
		E_LABELLED = 1,
		E_UNLABELLED = 2
	};

/** @brief Class InputParser is a templated class used to
 * maintain the reading/parsing/providing of examples.
 *
 * Parsing is done in a thread separate from the learner.
 *
 * Note that parsing is not done directly by this class,
 * but by the Streaming*File classes. This class only calls
 * the required get_vector* functions from the StreamingFile
 * object. (Exactly which function should be called is set
 * through the set_read_vector* functions)
 *
 * The template type should be the type of feature vector the
 * parser should return. Eg. InputParser<float32_t> means it
 * will expect a float32_t* vector to be returned from the get_vector
 * function. Other parameters returned are length of feature vector
 * and the label, if applicable.
 *
 * If the vectors cannot be directly represented as say float32_t*
 * one can instantiate eg. InputParser<VwExample> and it looks for
 * a get_vector function which returns a VwExample, which may contain
 * any kind of data, including label, vector, weights, etc. It is then
 * up to the external algorithm to handle such objects.

 * The parser should first be started with a call to the start_parser()
 * function which starts a new thread for continuous parsing of examples.
 *
 * Parsing is done through the ParseBuffer object, which in its
 * current implementation is a ring of a specified number of examples.
 * It is the task of the InputParser object to ensure that this ring
 * is being updated with new parsed examples.
 *
 * InputParser provides mainly the get_next_example function which
 * returns the next example from the ParseBuffer object to the caller
 * (usually a StreamingFeatures object). When one is done using
 * the example, finalize_example() should be called, leaving the
 * spot free for a new example to be loaded.
 *
 * The parsing thread should be joined with a call to end_parser().
 * exit_parser() may be used to cancel the parse thread if needed.
 *
 * Options are provided for automatic SG_FREEing of example objects
 * after each finalize_example() and also on InputParser destruction.
 * They are set through the set_free_vector* functions.
 * Do not free vectors on finalize_example() if you intend to reuse the
 * same vector memory locations for different examples.
 * Do not free vectors on destruction if you are bound to free them
 * manually later.
 */
template <class T> class InputParser
{
public:

    /**
     * Constructor
     *
     */
    InputParser();

    /**
     * Destructor
     *
     */
    ~InputParser();

    /**
     * Initializer
     *
     * Sets initial or default values for members.
     * is_example_used is initialized to EMPTY.
     * example_type is LABELLED by default.
     *
     * @param input_file StreamingFile object
     * @param is_labelled Whether example is labelled or not (bool), optional
     * @param size Size of the buffer in number of examples
     */
    void init(std::shared_ptr<StreamingFile> input_file, bool is_labelled = true, int32_t size = PARSER_DEFAULT_BUFFSIZE);

    /**
     * Test if parser is running.
     *
     * @return true if running, false otherwise.
     */
    bool is_running();

    /**
     * Get number of features from example.
     * Currently reads first line of input to infer.
     *
     * @return Number of features
     */
    int32_t get_number_of_features() { return number_of_features; }

    /**
     * Sets the function used for reading a vector from
     * the file.
     *
     * The function must be a member of CStreamingFile,
     * taking a T* as input for the vector, and an int for
     * length, setting both by reference. The function
     * returns void.
     *
     * The argument is a function pointer to that function.
     */
    void set_read_vector(void (StreamingFile::*func_ptr)(T* &vec, int32_t &len));

    /**
     * Sets the function used for reading a vector and
     * label from the file.
     *
     * The function must be a member of CStreamingFile,
     * taking a T* as input for the vector, an int for
     * length, and a float for the label, setting all by
     * reference. The function returns void.
     *
     * The argument is a function pointer to that function.
     */
    void set_read_vector_and_label(void (StreamingFile::*func_ptr)(T* &vec, int32_t &len, float64_t &label));

    /**
     * Gets feature vector, length and label.
     * Sets their values by reference.
     * Uses method for reading the vector defined in CStreamingFile.
     *
     * @param feature_vector Pointer to feature vector
     * @param length Features in vector
     * @param label Label of example
     *
     * @return 1 on success, 0 on failure.
     */
    int32_t get_vector_and_label(T* &feature_vector,
                     int32_t &length,
                     float64_t &label);

    /**
     * Gets feature vector and length by reference.
     * Assumes examples are unlabelled.
     * Uses method for reading the vector defined in CStreamingFile.
     *
     * @param feature_vector Pointer to feature vector
     * @param length Features in vector
     *
     * @return 1 on success, 0 on failure
     */
    int32_t get_vector_only(T* &feature_vector, int32_t &length);

    /**
     * Sets whether to SG_FREE() the vector explicitly
     * after it has been used
     *
     * @param free_vec whether to SG_FREE() or not, bool
     */
    void set_free_vector_after_release(bool free_vec);

    /**
     * Sets whether to free all vectors that were
     * allocated in the ring upon destruction of the ring.
     *
     * @param destroy free all vectors on destruction
     */
    void set_free_vectors_on_destruct(bool destroy);

    /**
     * Starts the parser, creating a new thread.
     *
     * main_parse_loop is the parsing method.
     */
    void start_parser();

    /**
     * Main parsing loop. Reads examples from source and stores
     * them in the buffer.
     *
     * @param params 'this' object
     *
     * @return NULL
     */
    void* main_parse_loop(void* params);


    /**
     * Copy example into the buffer.
     *
     * @param ex Example to be copied.
     */
    void copy_example_into_buffer(Example<T>* ex);

    /**
     * Retrieves the next example from the buffer.
     *
     *
     * @return The example pointer.
     */
    Example<T>* retrieve_example();

    /**
     * Gets the next example, assuming it to be labelled.
     *
     * Waits till retrieve_example returns a valid example, or
     * returns if reading is done already.
     *
     * @param feature_vector Feature vector pointer
     * @param length Length of feature vector
     * @param label Label of example
     *
     * @return 1 if an example could be fetched, 0 otherwise
     */
    int32_t get_next_example(T* &feature_vector,
                 int32_t &length,
                 float64_t &label);

    /**
     * Gets the next example, assuming it to be unlabelled.
     *
     * @param feature_vector
     * @param length
     *
     * @return 1 if an example could be fetched, 0 otherwise
     */
    int32_t get_next_example(T* &feature_vector,
                 int32_t &length);

    /**
     * Finalize the current example, indicating that the buffer
     * position it occupies may be overwritten by the parser.
     *
     * Should be called when the example has been processed by the
     * external algorithm.
     */
    void finalize_example();

    /**
     * End the parser, waiting for the parse thread to complete.
     *
     */
    void end_parser();

    /** Terminates the parsing thread
     */
    void exit_parser();

    /**
     * Returns the size of the examples ring
     *
     * @return ring size in terms of number of examples
     */
    int32_t get_ring_size() { return ring_size; }

private:
    /**
     * Entry point for the parse thread.
     *
     * @param params this object
     *
     * @return NULL
     */
    static void* parse_loop_entry_point(void* params);

public:
    bool parsing_done;	/**< true if all input is parsed */
    bool reading_done;	/**< true if all examples are fetched */

    E_EXAMPLE_TYPE example_type; /**< LABELLED or UNLABELLED */

protected:
    /**
     * This is the function pointer to the function to
     * read a vector from the input.
     *
     * It is called while reading a vector.
     */
    void (StreamingFile::*read_vector) (T* &vec, int32_t &len);

    /**
     * This is the function pointer to the function to
     * read a vector and label from the input.
     *
     * It is called while reading a vector and a label.
     */
    void (StreamingFile::*read_vector_and_label) (T* &vec, int32_t &len, float64_t &label);

    /// Input source, StreamingFile object
    std::shared_ptr<StreamingFile> input_source;

    /// Thread in which the parser runs
	std::thread parse_thread;

    /// The ring of examples, stored as they are parsed
    std::shared_ptr<ParseBuffer<T>> examples_ring;

    /// Number of features in dataset (max of 'seen' features upto point of access)
    int32_t number_of_features;

    /// Number of vectors parsed
    int32_t number_of_vectors_parsed;

    /// Number of vectors used by external algorithm
    int32_t number_of_vectors_read;

    /// Example currently being used
    Example<T>* current_example;

    /// Feature vector of current example
    T* current_feature_vector;

    /// Label of current example
    float64_t current_label;

    /// Number of features in current example
    int32_t current_len;

    /// Whether to SG_FREE() vector after it is used
    bool free_after_release;

    /// Size of the ring of examples
    int32_t ring_size;

    /// Mutex which is used when getting/setting state of examples (whether a new example is ready)
	std::mutex examples_state_lock;

    /// Condition variable to indicate change of state of examples
	std::condition_variable examples_state_changed;

	/// Flag that indicate that the parsing thread should continue reading
	alignas(CPU_CACHE_LINE_SIZE) std::atomic_bool keep_running;

};

template <class T>
    void InputParser<T>::set_read_vector(void (StreamingFile::*func_ptr)(T* &vec, int32_t &len))
{
    // Set read_vector to point to the function passed as arg
    read_vector=func_ptr;
}

template <class T>
    void InputParser<T>::set_read_vector_and_label(void (StreamingFile::*func_ptr)(T* &vec, int32_t &len, float64_t &label))
{
    // Set read_vector_and_label to point to the function passed as arg
    read_vector_and_label=func_ptr;
}

template <class T>
    InputParser<T>::InputParser()
{
	examples_ring = nullptr;
	parsing_done=true;
	reading_done=true;
	keep_running.store(false, std::memory_order_release);
}

template <class T>
    InputParser<T>::~InputParser()
{
}

template <class T>
    void InputParser<T>::init(std::shared_ptr<StreamingFile> input_file, bool is_labelled, int32_t size)
{
    input_source = input_file;
    example_type = is_labelled ? E_LABELLED : E_UNLABELLED;
    examples_ring = std::make_shared<ParseBuffer<T>>(size);

    parsing_done = false;
    reading_done = false;
    number_of_vectors_parsed = 0;
    number_of_vectors_read = 0;

    current_len = -1;
    current_label = -1;
    current_feature_vector = NULL;

    free_after_release=true;
    ring_size=size;
}

template <class T>
    void InputParser<T>::set_free_vector_after_release(bool free_vec)
{
    free_after_release=free_vec;
}

template <class T>
    void InputParser<T>::set_free_vectors_on_destruct(bool destroy)
{
	examples_ring->set_free_vectors_on_destruct(destroy);
}

template <class T>
    void InputParser<T>::start_parser()
{
	SG_TRACE("entering InputParser::start_parser()");
    if (is_running())
    {
        error("Parser thread is already running! Multiple parse threads not supported.");
    }

    SG_TRACE("creating parse thread");
    if (examples_ring)
		examples_ring->init_vector();
	keep_running.store(true, std::memory_order_release);
	parse_thread = std::thread(&parse_loop_entry_point, this);

    SG_TRACE("leaving InputParser::start_parser()");
}

template <class T>
    void* InputParser<T>::parse_loop_entry_point(void* params)
{
    ((InputParser *) params)->main_parse_loop(params);

    return NULL;
}

template <class T>
    bool InputParser<T>::is_running()
{
	SG_TRACE("entering InputParser::is_running()");
    bool ret;
	std::lock_guard<std::mutex> lock(examples_state_lock);

    if (parsing_done)
        if (reading_done)
            ret = false;
        else
            ret = true;
    else
        ret = false;

    SG_TRACE("leaving InputParser::is_running(), returning {}", ret);
    return ret;
}

template <class T>
    int32_t InputParser<T>::get_vector_and_label(T* &feature_vector,
                              int32_t &length,
                              float64_t &label)
{
    (input_source.get()->*read_vector_and_label)(feature_vector, length, label);

    if (length < 1)
    {
        // Problem reading the example
        return 0;
    }

    return 1;
}

template <class T>
    int32_t InputParser<T>::get_vector_only(T* &feature_vector,
                         int32_t &length)
{
    (input_source.get()->*read_vector)(feature_vector, length);

    if (length < 1)
    {
        // Problem reading the example
        return 0;
    }

    return 1;
}

template <class T>
    void InputParser<T>::copy_example_into_buffer(Example<T>* ex)
{
    examples_ring->copy_example(ex);
}

template <class T> void* InputParser<T>::main_parse_loop(void* params)
{
    // Read the examples into current_* objects
    // Instead of allocating mem for new objects each time
    InputParser* this_obj = (InputParser *) params;
    this->input_source = this_obj->input_source;

    while (keep_running.load(std::memory_order_acquire))
	{
		std::unique_lock<std::mutex> lock(examples_state_lock);

		if (parsing_done)
		{
			return NULL;
		}
		lock.unlock();

		current_example = examples_ring->get_free_example();
		current_feature_vector = current_example->fv;
		current_len = current_example->length;
		current_label = current_example->label;

		if (example_type == E_LABELLED)
			get_vector_and_label(current_feature_vector, current_len, current_label);
		else
			get_vector_only(current_feature_vector,	current_len);

		if (current_len < 0)
		{
			lock.lock();
			parsing_done = true;
			examples_state_changed.notify_one();
			return NULL;
		}

		current_example->label = current_label;
		current_example->fv = current_feature_vector;
		current_example->length = current_len;

		examples_ring->copy_example(current_example);
		lock.lock();
		number_of_vectors_parsed++;
		examples_state_changed.notify_one();
	}
    return NULL;
}

template <class T> Example<T>* InputParser<T>::retrieve_example()
{
    /* This function should be guarded by mutexes while calling  */
    Example<T> *ex;

    if (parsing_done)
    {
        if (number_of_vectors_read == number_of_vectors_parsed)
        {
            reading_done = true;
            /* Signal to waiting threads that no more examples are left */
			examples_state_changed.notify_one();
            return NULL;
        }
    }

    if (number_of_vectors_parsed <= 0)
        return NULL;

    if (number_of_vectors_read == number_of_vectors_parsed)
    {
        return NULL;
    }

    ex = examples_ring->get_unused_example();
    number_of_vectors_read++;

    return ex;
}

template <class T> int32_t InputParser<T>::get_next_example(T* &fv,
        int32_t &length, float64_t &label)
{
    /* if reading is done, no more examples can be fetched. return 0
       else, if example can be read, get the example and return 1.
       otherwise, wait for further parsing, get the example and
       return 1 */

    Example<T> *ex;

    while (keep_running.load(std::memory_order_acquire))
    {
        if (reading_done)
            return 0;

		std::unique_lock<std::mutex> lock(examples_state_lock);
        ex = retrieve_example();

        if (ex == NULL)
        {
            if (reading_done)
            {
                /* No more examples left, return */
                return 0;
            }
            else
            {
                /* Examples left, wait for one to become ready */
				examples_state_changed.wait(lock);
                continue;
            }
        }
        else
        {
            /* Example ready, return the example */
            break;
        }
    }

    fv = ex->fv;
    length = ex->length;
    label = ex->label;

    return 1;
}

template <class T>
    int32_t InputParser<T>::get_next_example(T* &fv, int32_t &length)
{
    float64_t label_dummy;

    return get_next_example(fv, length, label_dummy);
}

template <class T>
    void InputParser<T>::finalize_example()
{
    examples_ring->finalize_example(free_after_release);
}

template <class T> void InputParser<T>::end_parser()
{
	SG_TRACE("entering InputParser::end_parser");
	SG_TRACE("joining parse thread");
	if (parse_thread.joinable())
		parse_thread.join();
    SG_TRACE("leaving InputParser::end_parser");
}

template <class T> void InputParser<T>::exit_parser()
{
	SG_TRACE("cancelling parse thread");
	keep_running.store(false, std::memory_order_release);
	examples_state_changed.notify_one();
	if (parse_thread.joinable())
		parse_thread.join();
}
}

#endif // __INPUTPARSER_H__
