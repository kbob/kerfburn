import cPickle as pickle
import math


class GCodeException(Exception):
    pass


class ParameterSet(object):

    def __init__(self):

        self.dict = {}

    @staticmethod
    def load(file):

        pickle.load(file)

    def save(self, file):

        pickle.dump(self, file)

    def __getitem__(self, index):

        index = self.check_index(index)
        return self.dict.get(index, 0)

    def __setitem__(self, index, value):
        
        index = self.check_index(index)
        self.dict[index] = value

    def check_index(self, index):

        if isinstance(index, float):
            key = int(math.floor(index + 0.0002))
            if not (key - 0.0001 <= index <= key + 0.0001):
                m = 'parameter index %r is not close to an integer' % index
                raise GCodeException(m)
            index = key
        elif not isinstance(index, int):
            m = 'parameter index %r is not numeric' % index
            raise GCodeException(m)
                        
        if not (1 <= index <= 5399):
            m = 'parameter index %r is not in range 0..5399' % index
            raise GCodeException(m)
        return index
