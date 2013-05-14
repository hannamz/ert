#  Copyright (C) 2011  Statoil ASA, Norway. 
#   
#  The file 'ensemblefetcher.py' is part of ERT - Ensemble based Reservoir Tool. 
#   
#  ERT is free software: you can redistribute it and/or modify 
#  it under the terms of the GNU General Public License as published by 
#  the Free Software Foundation, either version 3 of the License, or 
#  (at your option) any later version. 
#   
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#  FITNESS FOR A PARTICULAR PURPOSE.   
#   
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
#  for more details. 



from fetcher import PlotDataFetcherHandler
from ert_gui.pages.config.parameters.parametermodels import FieldModel, SummaryModel, KeywordModel, DataModel
import ert.ert.ertwrapper as ertwrapper
import ert.ert.enums as enums
from PyQt4.QtGui import QWidget, QFormLayout, QSpinBox, QComboBox
from PyQt4.QtCore import SIGNAL
from ert.ert.erttypes import time_t, time_vector
import numpy
from ert.util.node_id import *
from ert.util.tvector import DoubleVector
import datetime
class EnsembleFetcher(PlotDataFetcherHandler):
    """A data fetcher for ensemble parameters."""

    def __init__(self):
        PlotDataFetcherHandler.__init__(self)
        
        self.field_configuration = FieldConfigurationWidget()
        self.summary_configuration = SummaryConfigurationWidget()
        self.keyword_configuration = KeywordConfigurationWidget()
        self.data_configuration = DataConfigurationWidget()

        def emitter():
            self.emit(SIGNAL('dataChanged()'))

        self.connect(self.field_configuration, SIGNAL('configurationChanged()'), emitter)
        self.connect(self.summary_configuration, SIGNAL('configurationChanged()'), emitter)
        self.connect(self.keyword_configuration, SIGNAL('configurationChanged()'), emitter)
        self.connect(self.data_configuration, SIGNAL('configurationChanged()'), emitter)

        
        
    def isHandlerFor(self, ert, key):
        return ert.main.ensemble_config.has_key( key)


    def fetch(self, ert, key, parameter, data, comparison_fs):
        data.x_data_type = "time"
        fs = ert.main.get_fs
        config_node = ert.main.ensemble_config.get_node(key)
        node = config_node.alloc_node
        comp_node = config_node.alloc_node
        num_realizations = ert.main.ens_size
        var_type_set = False
        user_data = parameter.getUserData()

        if user_data is None:
            return False

        key_index = None
        if user_data.has_key('key_index'):
            key_index = user_data['key_index']

        if parameter.getType() == FieldModel.TYPE:
            field_position = user_data['field_position']
            field_config = config_node.field_model
            if field_config.ijk_active(field_position[0] - 1, field_position[1] - 1, field_position[2] - 1):
                key_index = "%i,%i,%i" % (field_position[0], field_position[1], field_position[2])
                data.setKeyIndexIsIndex(True)
            else:
                return False
            var_type = enums.enkf_var_type.PARAMETER
            var_type_set = True
        elif parameter.getType() == DataModel.TYPE:
            data_index = user_data['data_index']
            key_index = "KEY:%d" % data_index
            data.setKeyIndexIsIndex(True)
            var_type = enums.enkf_var_type.PARAMETER
            var_type_set = True

        if var_type_set == False:
            var_type = enums.enkf_var_type.DYNAMIC_RESULT
        
        data.setKeyIndex(key_index)
        state_list = [user_data['state']]
        if state_list[0] == enums.ert_state_enum.BOTH:
            state_list = [enums.ert_state_enum.FORECAST, enums.ert_state_enum.ANALYZED]

        for member in range(0, num_realizations):
            data.x_data[member] = []
            data.y_data[member] = []
            x_time = data.x_data[member]
            y = data.y_data[member]
            
            if not comparison_fs is None:
                data.x_comp_data[member] = []
                data.y_comp_data[member] = []
                x_comp_time = data.x_comp_data[member]
                y_comp = data.y_comp_data[member]

            stop_time = ert.main.get_history_length

            for state in state_list:
                if not node.vector_storage:
                    if fs.has_vector(key, var_type.value(), member, state.value()):
                        time_map = fs.get_time_map
                        fs.fread_vector(key, var_type.value(), member, state.value())
                        victor = DoubleVector()
                        value = node.user_get_vector(fs, key, member, state.value(), victor)
                        if value == 1:
                            for step in range(1, stop_time):
                                sim_time1 = time_map.iget(step-1)
                                jaja = time_t(sim_time1)
                                sim_time = jaja.datetime().toordinal()
                                print "Her", sim_time, step, victor[step-1]
                                data.checkMaxMin(sim_time)
                                data.checkMaxMinY(victor[step])
                                x_time.append(sim_time)
                                y.append(victor[step])
                                print len(victor), "Not valid: ", key, member, step, victor[step], sim_time

                    
                        else:
                            print "Not valid: ", key, member, step
                        
                    if not comparison_fs is None:
                        if comparison_fs.has_node(key, step, member, state.value()):
                            time_map = comparison_fs.get_time_map
                            sim_time = time_map.iget(step)
                            print "Is this invoked 2"#sim_time
                            comparison_fs.fread_node(comp_node, var_type.value(), step, member, state.value())
                            valid = ertwrapper.c_double()
                            value = comp_node.user_get(comparison_fs, key, step, member, state.value(), ertwrapper.byref(valid))
                            if valid.value == 1:
                                #data.checkMaxMin(sim_time)
                                #data.checkMaxMinY(value)
                                x_comp_time.append(sim_time)
                                y_comp.append(value)
                            #else:
                            #    print "Not valid: ", key, member, step, key_index

                else:
                    for step in range(1, stop_time + 1):
                        if not fs.has_node(key, var_type.value(), step, member, state.value()):
                            time_map = fs.get_time_map
                            sim_time1 = time_map.iget(step)
                            jaja = time_t(sim_time1)
                            sim_time = jaja.datetime().toordinal()
                            #sim_time = time_map.iget(step)
                            #print "Is this invoked 1", key, var_type.value(), step, member, state.value()
                            #fs.fread_node(key, var_type.value(), step, member, state.value())
                            valid = ertwrapper.c_double()
                            value = node.user_get(fs, key, step, member, state.value(), ertwrapper.byref(valid))
                            if value == 1:
                                print sim_time, valid.value
                                data.checkMaxMin(sim_time1)
                                data.checkMaxMinY(valid.value)
                                x_time.append(sim_time)
                                y.append(valid.value)
                        #else:
                        #    print "Not valid: ", key, member, step, key_index
                        
                        if not comparison_fs is None:
                            if comparison_fs.has_node(key, step, member, state.value()):
                                time_map = comparison_fs.get_time_map
                                sim_time = time_map.iget(step)
                                print "Is this invoked 2"#sim_time
                                comparison_fs.fread_node(comp_node, step, member, state.value())
                                valid = ertwrapper.c_double()
                                value = comp_node.user_get(comparison_fs, key_index, step, member, state.value(), ertwrapper.byref(valid))
                                if valid.value == 1:
                                    #data.checkMaxMin(sim_time)
                                    #data.checkMaxMinY(value)
                                    x_comp_time.append(sim_time)
                                    y_comp.append(value)
                            #else:
                            #    print "Not valid: ", key, member, step, key_index
                            
                            
                            data.x_data[member] = numpy.array([t.datetime() for t in x_time])
            data.y_data[member] = numpy.array(y)

            if not comparison_fs is None:
                data.x_comp_data[member] = numpy.array([t.datetime() for t in x_comp_time])
                data.y_comp_data[member] = numpy.array(y_comp)


        self._getObservations(ert, key, key_index, data)

        self._getRefCase(ert, key, data)

        #node.__del__
        #comp_node.__del__

        data.inverted_y_axis = False

    def _getObservations(self, ert, key, key_index, data):
        if not key_index is None:
            user_key = "%s:%s" % (key, key_index)
        else:
            user_key = key

        obs_count = ert.main.get_observation_count(user_key)
        print obs_count
        if obs_count > 0:
            obs_x = (time_t * obs_count)()
            obs_y = (ertwrapper.c_double * obs_count)()
            obs_std = (ertwrapper.c_double * obs_count)()
            ert.main.get_observations(user_key, obs_count, obs_x, obs_y, obs_std)

            print "Hit?"
            
            data.obs_x = numpy.array([t.datetime() for t in obs_x])
            data.obs_y = numpy.array(obs_y)
            data.obs_std_y = numpy.array(obs_std)
            data.obs_std_x = None

            #data.checkMaxMin(max(obs_x))
            #data.checkMaxMin(min(obs_x))

            #data.checkMaxMinY(max(obs_y))
            #data.checkMaxMinY(min(obs_y))


    def _getRefCase(self, ert, key, data):
        ecl_config = ert.main.ecl_config
        ecl_sum = ecl_config.get_refcase

        if(ecl_sum.has_key(key)):
            ki = ecl_sum.get_general_var_index(key)
            x_data = ecl_sum.alloc_time_vector(True)
            y_data = ecl_sum.alloc_data_vector(ki, True)

            data.refcase_x = []
            data.refcase_y = []
            first = True
            for x in x_data:
                if not first:
                    data.refcase_x.append(x)
                    #data.checkMaxMin(x)
                else:
                    first = False #skip first element because of eclipse behavior

            first = True
            for y in y_data:
                if not first:
                    data.refcase_y.append(y)
                    data.checkMaxMinY(y)
                else:
                    first = False #skip first element because of eclipse behavior

            data.refcase_x = numpy.array([t.datetime() for t in data.refcase_x])
            data.refcase_y = numpy.array(data.refcase_y)

    def configure(self, parameter, context_data):
        self.parameter = parameter

        cw = self.getConfigurationWidget(context_data)
        if not cw is None:
            cw.setParameter(parameter)

    def getConfigurationWidget(self, context_data):
        if self.parameter.getType() == SummaryModel.TYPE:
            return self.summary_configuration
        elif self.parameter.getType() == KeywordModel.TYPE:
            key_index_list = context_data.getKeyIndexList(self.parameter.getName())
            self.keyword_configuration.setKeyIndexList(key_index_list)
            return self.keyword_configuration
        elif self.parameter.getType() == FieldModel.TYPE:
            bounds = context_data.field_bounds
            self.field_configuration.setFieldBounds(*bounds)
            return self.field_configuration
        elif self.parameter.getType() == DataModel.TYPE:
            index = context_data.gen_data_size
            self.data_configuration.setIndexBounds(index)
            return self.data_configuration
        else:
            return None


#---------------------------------------------------------
# The following widgets are used to configure the
# different parameter types.
#---------------------------------------------------------

class ConfigurationWidget(QWidget):
    """An abstract configuration widget."""
    def __init__(self):
        QWidget.__init__(self)
        self.layout = QFormLayout()
        self.layout.setRowWrapPolicy(QFormLayout.WrapLongRows)

        self.stateCombo = QComboBox()

        for state in enums.ert_state_enum.values():
           self.stateCombo.addItem(state.name)

        self.stateCombo.setCurrentIndex(0)

        self.layout.addRow("State:", self.stateCombo)

        self.setLayout(self.layout)

        self.connect(self.stateCombo, SIGNAL('currentIndexChanged(QString)'), self.applyConfiguration)

    def addRow(self, label, widget):
        """Add another item to this widget."""
        self.layout.addRow(label, widget)

    def setParameter(self, parameter):
        """Set the parameter to configure."""
        self.parameter = parameter
        self.applyConfiguration(False)

    def getState(self):
        """State is common for all parameters."""
        selectedName = str(self.stateCombo.currentText())
        return enums.ert_state_enum.resolveName(selectedName)

    def emitConfigurationChanged(self, emit=True):
        """Emitted when a sub widget changes the state of the parameter."""
        if emit:
            self.emit(SIGNAL('configurationChanged()'))

    def applyConfiguration(self, emit=True):
        """Override! Set the user_data of self.paramater and optionally: emitConfigurationChanged()"""
        pass


class SummaryConfigurationWidget(ConfigurationWidget):
    def __init__(self):
        ConfigurationWidget.__init__(self)


    def applyConfiguration(self, emit=True):
        user_data = {'state': self.getState()}
        self.parameter.setUserData(user_data)
        self.emitConfigurationChanged(emit)

        
class FieldConfigurationWidget(ConfigurationWidget):
    def __init__(self):
        ConfigurationWidget.__init__(self)

        self.keyIndexI = QSpinBox()
        self.keyIndexI.setMinimum(1)

        self.addRow("i:", self.keyIndexI)

        self.keyIndexJ = QSpinBox()
        self.keyIndexJ.setMinimum(1)
        self.addRow("j:", self.keyIndexJ)

        self.keyIndexK = QSpinBox()
        self.keyIndexK.setMinimum(1)
        self.addRow("k:", self.keyIndexK)

        self.setFieldBounds(1, 1, 1)

        self.connect(self.keyIndexI, SIGNAL('valueChanged(int)'), self.applyConfiguration)
        self.connect(self.keyIndexJ, SIGNAL('valueChanged(int)'), self.applyConfiguration)
        self.connect(self.keyIndexK, SIGNAL('valueChanged(int)'), self.applyConfiguration)

    def setFieldBounds(self, i, j, k):
        self.keyIndexI.setMaximum(i)
        self.keyIndexJ.setMaximum(j)
        self.keyIndexK.setMaximum(k)

    def getFieldPosition(self):
        return (self.keyIndexI.value(), self.keyIndexJ.value(), self.keyIndexK.value())

    def setParameter(self, parameter):
        self.parameter = parameter
        self.applyConfiguration(False)

    def applyConfiguration(self, emit=True):
        user_data = {'state': self.getState(), 'field_position': self.getFieldPosition()}
        self.parameter.setUserData(user_data)
        self.emitConfigurationChanged(emit)


class KeywordConfigurationWidget(ConfigurationWidget):
    def __init__(self):
        ConfigurationWidget.__init__(self)
        self.keyIndexCombo = QComboBox()
        self.addRow("Key index:", self.keyIndexCombo)

        self.connect(self.keyIndexCombo, SIGNAL('currentIndexChanged(QString)'), self.applyConfiguration)

    def setKeyIndexList(self, list):
        self.disconnect(self.keyIndexCombo, SIGNAL('currentIndexChanged(QString)'), self.applyConfiguration)
        self.keyIndexCombo.clear()
        self.keyIndexCombo.addItems(list)
        self.connect(self.keyIndexCombo, SIGNAL('currentIndexChanged(QString)'), self.applyConfiguration)

    def setParameter(self, parameter):
        self.parameter = parameter
        self.applyConfiguration(False)

    def applyConfiguration(self, emit=True):
        user_data = {'state': self.getState(), 'key_index' : self.getKeyIndex()}
        self.parameter.setUserData(user_data)
        self.emitConfigurationChanged(emit)

    def getKeyIndex(self):
        return str(self.keyIndexCombo.currentText())

class DataConfigurationWidget(ConfigurationWidget):
    def __init__(self):
        ConfigurationWidget.__init__(self)

        self.keyIndex = QSpinBox()
        self.keyIndex.setMinimum(0)

        self.addRow("index:", self.keyIndex)

        self.setIndexBounds(0)

        self.connect(self.keyIndex, SIGNAL('valueChanged(int)'), self.applyConfiguration)

    def setIndexBounds(self, i):
        self.keyIndex.setMaximum(i)

    def getIndex(self):
        return self.keyIndex.value()

    def setParameter(self, parameter):
        self.parameter = parameter
        self.applyConfiguration(False)

    def applyConfiguration(self, emit=True):
        user_data = {'state': self.getState(), 'data_index': self.getIndex()}
        self.parameter.setUserData(user_data)
        self.emitConfigurationChanged(emit)




