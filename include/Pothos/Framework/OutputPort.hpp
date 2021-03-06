///
/// \file Framework/OutputPort.hpp
///
/// This file provides an interface for a worker's output port.
///
/// \copyright
/// Copyright (c) 2014-2016 Josh Blum
/// SPDX-License-Identifier: BSL-1.0
///

#pragma once
#include <Pothos/Config.hpp>
#include <Pothos/Object/Object.hpp>
#include <Pothos/Framework/DType.hpp>
#include <Pothos/Framework/Label.hpp>
#include <Pothos/Framework/BufferPool.hpp>
#include <Pothos/Framework/BufferChunk.hpp>
#include <Pothos/Framework/BufferManager.hpp>
#include <Pothos/Util/RingDeque.hpp>
#include <Pothos/Util/SpinLock.hpp>
#include <string>

namespace Pothos {

class WorkerActor;
class InputPort;

/*!
 * OutputPort provides methods to interact with a worker's output ports.
 */
class POTHOS_API OutputPort
{
public:

    //! Destructor
    ~OutputPort(void);

    /*!
     * Get the index number of this port.
     * An index of -1 means the port cannot be represented by an integer.
     * \return the index or -1
     */
    int index(void) const;

    //! Get the string name identifier for this port.
    const std::string &name(void) const;

    //! Get a displayable name for this port.
    const std::string &alias(void) const;

    //! Set the displayable alias for this port.
    void setAlias(const std::string &alias);

    //! Get the data type information for this port.
    const DType &dtype(void) const;

    //! Get the domain information for this port
    const std::string &domain(void) const;

    /*!
     * Get access to the stream buffer.
     * For non-stream ports, this returns an empty buffer chunk.
     */
    const BufferChunk &buffer(void) const;

    /*!
     * Get the number of elements available in the stream buffer.
     * The number of elements is the available bytes/dtype size.
     */
    size_t elements(void) const;

    /*!
     * Get the total number of elements produced from this port.
     * The value returned by this method will not change
     * until after execution of work() and propagateLabels().
     */
    unsigned long long totalElements(void) const;

    /*!
     * Get the total number of buffers produced from this port.
     * This value will increment immediately after postBuffer().
     * This call will also increment after every work execution
     * where elements are produced using the produce() method.
     */
    unsigned long long totalBuffers(void) const;

    /*!
     * Get the total number of labels produced from this port.
     * This value will increment immediately after postLabel().
     */
    unsigned long long totalLabels(void) const;

    /*!
     * Get the total number of messages posted to this port.
     * The value returned by this method will be incremented
     * immediately upon calling postMessage().
     */
    unsigned long long totalMessages(void) const;

    /*!
     * Produce elements from this port.
     * The number of elements specified must be less than
     * or equal to the number of elements available.
     * \param numElements the number of elements to produce
     */
    void produce(const size_t numElements);

    /*!
     * Remove all or part of an output buffer from the port;
     * without producing elements for downstream consumers.
     * This call allows the user to use the output buffer
     * for non-streaming purposes, such as an async message.
     * The number of bytes should be less than or equal to
     * the bytes available and a multiple of the element size.
     * \deprecated replaced by popElements(numElements)
     * \param numBytes the number of bytes to remove
     */
    void popBuffer(const size_t numBytes);

    /*!
     * Remove all or part of an output buffer from the port;
     * without producing elements for downstream consumers.
     * This call allows the user to use the output buffer
     * for non-streaming purposes, such as an async message.
     * The number of elements specified should be less than or
     * equal to the available number elements on this port.
     * \param numElements the number of elements to remove
     */
    void popElements(const size_t numElements);

    /*!
     * Get a buffer of a specified size in elements.
     * Some blocks may require buffers of arbitrary size
     * for use with postBuffer() and postMessage() calls.
     * The call will attempt to use the front buffer from
     * this output port and will fall-back to a reusable pool.
     * \param numElements the minimum buffer size
     * \return a buffer chunk with at least numElements
     */
    BufferChunk getBuffer(const size_t numElements);

    /*!
     * Post an output label to the subscribers on this port.
     * \param label the label to post
     */
    void postLabel(const Label &label);

    /*!
     * Post an output message to the subscribers on this port.
     * \param message the message to post
     */
    template <typename ValueType>
    void postMessage(ValueType &&message);

    /*!
     * Post an output buffer to the subscribers on this port.
     * This call allows external user-provided buffers to be used
     * in place of the normal stream buffer provided by buffer().
     * The number of elements to produce is determined from
     * the buffer's length field divided by the dtype size.
     * Do not call produce() when using postBuffer().
     * \param buffer the buffer to post
     */
    void postBuffer(const BufferChunk &buffer);

    /*!
     * Set a reserve requirement on this output port.
     * The reserve size ensures that when sufficient resources are available,
     * the buffer will contain at least the specified number of elements.
     * By default, each output port has a reserve of zero elements,
     * which means that the output port's buffer may be any size,
     * but not empty, depending upon the available resources.
     * Note that work() may still be called when the reserve is not met,
     * because the scheduler will only prevent work() from being called
     * when any given port has no available output elements.
     * \param numElements the number of elements to require
     */
    void setReserve(const size_t numElements);

    /*!
     * Is this port used for signaling in a signals + slots paradigm?
     */
    bool isSignal(void) const;

    /*!
     * Set read before write on this output port.
     * Read before write indicates that an element will be read from the input
     * before a corresponding element is written to an element from this output.
     *
     * This property implies that the input buffer can be used
     * as an output buffer duing a call to the work operation.
     * Also referred to as buffer inlining, the intention is to
     * keep more memory in cache by using the same buffer
     * for both reading and writing operations when possible.
     *
     * When this "read before write" property is enabled,
     * and the only reference to the buffer is held by the input port,
     * and the size of the input elements is the same as the output.
     * then the input buffer may be substituted for an output buffer.
     *
     * \param port the input port to borrow the buffer from
     */
    void setReadBeforeWrite(InputPort *port);

private:
    WorkerActor *_actor;

    //port configuration
    bool _isSignal;
    int _index;
    std::string _name;
    std::string _alias;
    DType _dtype;

    //state set in pre-work
    std::string _domain;
    BufferChunk _buffer;
    size_t _elements;

    //port stats
    unsigned long long _totalElements;
    unsigned long long _totalBuffers;
    unsigned long long _totalLabels;
    unsigned long long _totalMessages;

    //state changes from work
    size_t _pendingElements;
    size_t _reserveElements;
    std::vector<Label> _postedLabels;
    Util::RingDeque<BufferChunk> _postedBuffers;

    //counts work actions which we will use to establish activity
    size_t _workEvents;

    Util::SpinLock _bufferManagerLock;
    BufferManager::Sptr _bufferManager;

    Util::SpinLock _tokenManagerLock;
    BufferManager::Sptr _tokenManager; //used for message backpressure

    /////// buffer manager /////////
    void bufferManagerSetup(const BufferManager::Sptr &manager);
    bool bufferManagerEmpty(void);
    void bufferManagerFront(BufferChunk &);
    void bufferManagerPop(const size_t numBytes);
    void bufferManagerPush(Pothos::Util::SpinLock *mutex, const ManagedBuffer &buff);

    /////// token manager /////////
    void tokenManagerInit(void);
    bool tokenManagerEmpty(void);
    BufferChunk tokenManagerPop(void);
    void tokenManagerPop(const size_t numBytes);

    std::vector<InputPort *> _subscribers;
    InputPort *_readBeforeWritePort;
    bool _bufferFromManager;
    BufferPool _bufferPool;

    OutputPort(void);
    OutputPort(const OutputPort &){} // non construction-copyable
    OutputPort &operator=(const OutputPort &){return *this;} // non copyable
    friend class WorkerActor;
    friend class InputPort;
    void _postMessage(const Object &message);
};

} //namespace Pothos
