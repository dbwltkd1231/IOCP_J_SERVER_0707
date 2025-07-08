#pragma once

#include<iostream>
#include<atomic>

namespace Utility
{
	template<typename T>
	class LockFreeCircleQueue
	{
	private:
		int _queueMaxSize;// ���������δ� queueMaxSize-1��ŭ �������ִ�. ����ť�� �ϳ��� ������ ��������ν� full�� empty���¸� �����ϱ� ����.
		std::atomic<int> _inputIndex;
		std::atomic<int> _outputIndex;
		T* _buffer;
	public:
		LockFreeCircleQueue<T>()
		{
			_inputIndex = 0;
			_outputIndex = 0;
		}

		~LockFreeCircleQueue<T>()
		{
			delete[] _buffer;
		}

		void Construct(int queueMaxSize)
		{
			this->_queueMaxSize = queueMaxSize + 1;
			_buffer = new T[queueMaxSize + 1];
		}

		//���⼭ &&�� rvalue reference�� �ǹ��Ѵ�.
		// rvalue reference�� �ӽ� ��ü�� �����ϴµ� ���Ǹ�, std::move()�� �Բ� ����Ͽ� ��ü�� �������� �̵��� �������� �Ҷ� ���ȴ�.
		//����ȭ �̵��� ���� �߿��� �������Ѵ�.
		bool push(T&& data)
		{
			int currentInputIndex = _inputIndex.load(std::memory_order_acquire);
			int nextIndex = (currentInputIndex + 1) % _queueMaxSize;

			if (nextIndex == _outputIndex.load(std::memory_order_acquire))
			{
				std::cout << "Queue is full" << std::endl;
				return false;
			}

			_buffer[currentInputIndex] = std::move(data);
			_inputIndex.store(nextIndex, std::memory_order_release);// ������Ʈ �� release
			return true;
		}

		T pop()
		{
			int currentOutputIndex = _outputIndex.load(std::memory_order_acquire); // �ֽ� outputIndex�� �����ͼ� ���Ѵ�.

			if (currentOutputIndex == _inputIndex.load(std::memory_order_acquire))//�����͸� �д� load �۾����� ����Ͽ� ���� ����ȭ.
			{
				std::cout << "Queue is empty" << std::endl;
				return T();
			}

			T data = std::move(_buffer[currentOutputIndex]);
			_outputIndex.store((currentOutputIndex + 1) % _queueMaxSize, std::memory_order_release);// �����͸� ������ ����� �� �ε����� ������Ʈ�� �� ���
			return data;
		}

		bool empty()
		{
			return _inputIndex.load(std::memory_order_acquire) == _outputIndex.load(std::memory_order_acquire);
		}

		int size()
		{
			auto input = _inputIndex.load(std::memory_order_acquire);
			auto output = _outputIndex.load(std::memory_order_acquire);

			if (input >= output)
				return input - output;
			else
				return _queueMaxSize - _outputIndex + input;
		}

		int capacity()
		{
			return _queueMaxSize;
		}

		void clear()
		{
			_inputIndex.store(0, std::memory_order_release);
			_outputIndex.store(0, std::memory_order_release);
		}

		void print()
		{
			std::cout << "Input Index: " << _inputIndex << ", Output Index: " << _outputIndex << std::endl;
			for (int i = 0; i < size(); i++)
			{
				std::cout << _buffer[(_outputIndex + i) % _queueMaxSize] << " ";
			}
			std::cout << std::endl;
		}

		T Front()
		{
			return _buffer[_outputIndex.load(std::memory_order_acquire)];
		}
	};
}

/*

shard_ptrŸ���� T�� ���� ��ü�� �Ϻ� ���ǵǴ� ������ �߻���.

LockFreeCircleQueue�� shared_ptr<T> Ÿ���� ������ ��, ���ο��� new T[queueMaxSize]�� �迭�� �����ϴ� �����̱� ��������
- shared_ptr<EventWorker> �迭�� ���� nullptr�� �ʱ�ȭ��
- ���� push(std::move(shared_ptr))�� �ص�, ����/�̵��� ����� ���� �ʰų�
- pop()���� ���� ��ü�� nullptr�̾ ��� �� ũ���� �߻�


- ť�� EventWorker �� Ÿ������ ���� (��, LockFreeCircleQueue<EventWorker>)
- �� new T[]�� ������ ��ü���� �ùٸ��� �����Ǹ�
- �� move�� copy�� �� Ÿ������ Ȯ���� �۵���
- �� ��ü ����, �ʱ�ȭ, ������ ���� �ذ�


*/