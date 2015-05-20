#ifndef __SUBJECT_H__
#define __SUBJECT_H__

#include <memory>

template <class T>
class Subject;

template <class T>
class Observer
{
public:
	Observer() {}
	virtual ~Observer() {}
	virtual void onEvent(T what, const Subject<T>& sender) = 0;
};

template <class T>
class Subject
{
public:
	Subject() {}
	virtual ~Subject() {}

	void attach(Observer<T> &observer)
	{
		m_observers.push_back(&observer);
	}

	void notify(T what)
	{
		std::vector<Observer<T> *>::iterator it;
		for (it = m_observers.begin(); it != m_observers.end(); it++)
			(*it)->onEvent(what, *this);
	}

private:
	std::vector<Observer<T> *> m_observers;
};

#endif //__SUBJECT_H__